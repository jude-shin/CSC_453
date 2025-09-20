#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include "family.h"

#define BUFFER_SIZE 2048
#define OUT_PERMS 0644 // permissions for the output file

int main(int argc, char *argv[]) {
	// forking
	pid_t pid = 0;

	// file desc. for parent -> child1
	int parent_child1_fd[2]; 
	if (pipe(parent_child1_fd) == -1) {
		perror("[parent] error making pipe parent_child1_fd");
		return EXIT_FAILURE;
	}

	// create child1
	pid = fork();
	if (pid == -1) { 
		close(parent_child1_fd[0]);
		close(parent_child1_fd[1]);
		perror("[parent] error forking child1");
		return EXIT_FAILURE;
	}

	if (pid == 0) {
		// file desc. for child1 -> child2
		int child1_child2_fd[2]; 
		if (pipe(child1_child2_fd) == -1) {
			perror("[child1] error making pipe child1_child2_fd");
			return EXIT_FAILURE;
		}
		
		// create child2
		pid = fork();
		if (pid == -1) { 
			close(parent_child1_fd[0]);
			close(parent_child1_fd[1]);
			close(child1_child2_fd[0]);
			close(child1_child2_fd[1]);
			perror("[child1] error forking child2");
			return EXIT_FAILURE;
		}

		if (pid == 0)	{ // child2 code
			// we don't need to access pipe parent_child1_fd as child2
			close(parent_child1_fd[0]);
			close(parent_child1_fd[1]);

			// keep the "read side" of child1_child2_fd
			// (therefore, close the "write side" of that pipe
			close(child1_child2_fd[1]);

			// --------------------------------
			// TODO: make this a seperate function?
			// called redirect_to_file or something like that
			char buffer[BUFFER_SIZE];
			ssize_t n = 0;

			int output_fd = open("output", O_CREAT|O_WRONLY|O_TRUNC, OUT_PERMS); 
			if (output_fd == -1) {
				close(parent_child1_fd[0]);
				perror("[child2] could not open output file");
				return EXIT_FAILURE;
			}

			while ((n = read(child1_child2_fd[0], buffer, BUFFER_SIZE)) > 0) {
				if (write(output_fd, buffer, n) == -1) {
					close(child1_child2_fd[0]);
					close(output_fd);
					perror("[child2] error writing to output file");
					return EXIT_FAILURE;
				}
			}

			if (n == -1) {
				close(child1_child2_fd[0]);
				close(output_fd);
				perror("[child2] error reading from pipe parent_child1_fd");
				return EXIT_FAILURE;
			}
			// --------------------------------

			close(child1_child2_fd[1]);
			return EXIT_SUCCESS;
		}
		// TODO: get rid of the else statements?
		else { // child1 code
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
				return EXIT_FAILURE;
			}
			// change stdout (to be to the child1_child2_fd "write side")
			if (dup2(child1_child2_fd[1], STDOUT_FILENO) == -1) {
				close(parent_child1_fd[0]);
				close(child1_child2_fd[1]);
				return EXIT_FAILURE;
			}

			close(parent_child1_fd[0]);
			close(child1_child2_fd[1]);

			// execute stdin
			if (execlp("sort", "sort", "-r", NULL) == -1) {
				perror("[child1] error executing \"$sort -r\" command");
				return EXIT_FAILURE;
			}
		}
	}
	// TODO: get rid of the else statements?
	else { // parent code
		return parent(parent_child1_fd);
	}
}

