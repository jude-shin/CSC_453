#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#define BUFFER_SIZE 2048
#define OUT_PERMS 0644 // permissions for the output file

void child2(int *child1_child2_fd) {
	// keep the "read side" of child1_child2_fd
	// (therefore, close the "write side" of that pipe
	close(child1_child2_fd[1]);

	char buffer[BUFFER_SIZE];
	ssize_t n = 0;

	int output_fd = open("output", O_CREAT|O_WRONLY|O_TRUNC, OUT_PERMS); 
	if (output_fd == -1) {
		close(child1_child2_fd[0]);
		perror("[child2] could not open output file");
		exit(EXIT_FAILURE);
	}

	while ((n = read(child1_child2_fd[0], buffer, BUFFER_SIZE)) > 0) {
		if (write(output_fd, buffer, n) == -1) {
			close(child1_child2_fd[0]);
			close(output_fd);
			perror("[child2] error writing to output file");
			exit(EXIT_FAILURE);
		}
	}

	if (n == -1) {
		close(child1_child2_fd[0]);
		close(output_fd);
		perror("[child2] error reading from pipe child1_child2_fd");
		exit(EXIT_FAILURE);
	}

	close(child1_child2_fd[1]);
	close(output_fd);
	exit(EXIT_SUCCESS);
}

void child1(int *parent_child1_fd, int *child1_child2_fd) {
	// keep the "read side" of parent_child1_fd
	// (therefore, close the "write side" of that pipe)
	close(parent_child1_fd[1]);

	// keep the "write side" of child1_child2_fd
	// (therefore, close the "read side" of that pipe)
	close(child1_child2_fd[0]);

	// change stdin (to be from the parent_child1_fd "read side")
	if (dup2(parent_child1_fd[0], STDIN_FILENO) == -1) {
		close(parent_child1_fd[0]);
		close(child1_child2_fd[1]);
		exit(EXIT_FAILURE);
	}
	// change stdout (to be to the child1_child2_fd "write side")
	if (dup2(child1_child2_fd[1], STDOUT_FILENO) == -1) {
		close(parent_child1_fd[0]);
		close(child1_child2_fd[1]);
		exit(EXIT_FAILURE);
	}

	close(parent_child1_fd[0]);
	close(child1_child2_fd[1]);

	// execute stdin
	if (execlp("sort", "sort", "-r", NULL) == -1) {
		perror("[child1] error executing \"$sort -r\" command");
		exit(EXIT_FAILURE);
	}
}

void parent(int *parent_child1_fd) {
	// keep the "write side" of parent_child1_fd
	// (therefore, close the "read side" of that pipe)
	close(parent_child1_fd[0]);

	// redirect stdout to the file descriptor that is the "write" end of 
	// the parent to child 1 pipe
	if (dup2(parent_child1_fd[1], STDOUT_FILENO) == -1) {
		close(parent_child1_fd[1]); 
		exit(EXIT_FAILURE);
	}
	close(parent_child1_fd[1]); 

	// the output which goes to stdout is now the write end of the pipe
	// when ls is executed, the output will flow through that pipe
	if (execlp("ls", "ls", NULL) == -1) {
		perror("[parent] error executing \"$ls\" command");
		exit(EXIT_FAILURE);
	}
}
