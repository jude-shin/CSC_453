#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#define BUFFER_SIZE 2048
#define OUT_PERMS 0644 // permissions for the output file

int parent(int *parent_child1_fd) {
	close(parent_child1_fd[0]);

	// redirect stdout to the file descriptor that is the "write" end of 
	// the parent to child 1 pipe
	if (dup2(parent_child1_fd[1], STDOUT_FILENO) == -1) {
		close(parent_child1_fd[1]); 
		return EXIT_FAILURE;
	}
	close(parent_child1_fd[1]); 

	// the output which goes to stdout is now the write end of the pipe
	// when ls is executed, the output will flow through that pipe
	if (execlp("ls", "ls", NULL) == -1) {
		perror("[parent] error executing \"$ls\" command");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
