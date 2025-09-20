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

			// TODO: ask if you should close the child process before
			// the function is called, or if you should close them as soon as possible
			child2(child1_child2_fd);
		}
		// TODO: get rid of the else statements?
		else { // child1 code
			child1(parent_child1_fd, child1_child2_fd);
		}
	}
	
	// TODO: get rid of the else statements?
	else { // parent code
		// TODO: ask if you should close the child process before
		// the function is called, or if you should close them as soon as possible
		parent(parent_child1_fd);
	}
}

