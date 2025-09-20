#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define BUFFER_SIZE 2048

// parent creates pipe ptoc1
// parent creates pipe c1top2
// parent creates pipe c2toout

// parent forks child child1
// parent executes '$ls' command
// parent sends through pipe ptoc1

// child1 forks child2
// child1 reads ptoc1
// child1 executes '$sort -r' command on the ptoc1 data
// child1 sends through to pipe c1toc2

// child2 (creates no more children)
// child2 reads c1toc2
// child2 creates a new file called output
// child2 sends through to output

// make sure to close all pipes, (even in error)
// make sure to close all file descriptors (even in error)


// small example: skip child1


int main(int argc, char *argv[]) {
	// forking
	pid_t pid = 0;
	int wait_status = 0;

	// file descriptors
	int parent_child1_fd[2]; // file desc. for parent -> child1
	if (pipe(parent_child1_fd) == -1) {
		perror("[parent] error making pipe parent_child1_fd");
		return EXIT_FAILURE;
	}
	// int c1toc2[2]; // file desc. for child1 -> child2


	// create child2
	pid = fork();
	if (pid == -1) { 
		close(parent_child1_fd[0]);
		close(parent_child1_fd[1]);
		perror("[parent] error forking child2");
		return EXIT_FAILURE;
	}

	if (pid == 0) { // child2 code
		// Close "write end" file descriptor (OR OTHER FILE DESCRIPTORS THAT ARE IRRELEVENT)
		close(parent_child1_fd[1]);

		// Get information from the parent
		char buffer[BUFFER_SIZE];
		ssize_t n = 0;
		int output_fd = open("output", O_CREAT|O_WRONLY|O_TRUNC, 0644);
		if (output_fd == -1) {
			close(parent_child1_fd[0]);
			perror("[child2] could not open output file");
			return EXIT_FAILURE;
		}

		while ((n = read(parent_child1_fd[0], buffer, BUFFER_SIZE)) > 0) {
			if (write(output_fd, buffer, n) == -1) {
				close(parent_child1_fd[0]);
				close(output_fd);
				perror("[child2] error writing to output file");
				return EXIT_FAILURE;
			}
		}

		if (n == -1) {
			close(parent_child1_fd[0]);
			close(output_fd);
			perror("[child2] error reading from pipe parent_child1_fd");
			return EXIT_FAILURE;
		}

		close(parent_child1_fd[0]);
		close(output_fd);

		return EXIT_SUCCESS;
	}
	else { // parent code
		close(parent_child1_fd[0]); // close the read end of the file descriptor

		char message[] = "hello world!\n";

		if (write(parent_child1_fd[1], message, sizeof(char)*strlen(message)) == -1) {
			close(parent_child1_fd[1]);
			perror("[parent] error writing message to pipe parent_child1_fd");
			return EXIT_FAILURE;
		}

		// // wait for the child process to finish
		// waitpid(pid, &wait_status, 0);

		// if (!WIFEXITED(wait_status)) {
		// 	close(parent_child1_fd[1]);
		// 	perror("[parent] error waiting for child2");
		// 	return EXIT_FAILURE;
		// }

		close(parent_child1_fd[1]); // data will only send EOF when the pipe is closed on the write end

		printf("program finished successfully...\n");
		return EXIT_SUCCESS;
	}
}
