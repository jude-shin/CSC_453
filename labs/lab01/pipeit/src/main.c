#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include "family.h"

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

			// TODO: ASK Ask ask if I should close the write end here (asap), or
			// in the function (as it is now... for readablity)
			child2(child1_child2_fd);
		}

		// TODO: get rid of the else statements?
		child1(parent_child1_fd, child1_child2_fd);
	}

	// TODO: ASK Ask ask if I should close the write end here (asap), or
	// in the function (as it is now... for readablity)
	parent(parent_child1_fd);
}

