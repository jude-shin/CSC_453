#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

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
	// int ptoc1[2]; // file desc. for parent -> child1
	// int c1toc2[2]; // file desc. for child1 -> child2
	// int c2toout[2]; // file desc. for child2 -> output (a file)

	int fd[2]; // file descriptor for testing
	if (pipe(fd) == -1) {
		return EXIT_FAILURE;
	}

	// create child2
	pid = fork();
	if (pid == -1) { 
		close(fd[0]);
		close(fd[1]);
		return EXIT_FAILURE;
	}

	if (pid == 0) { // child2 code
		// do whatever you want in the child code
		// Close "write end" file descriptor (OR OTHER FILE DESCRIPTORS THAT ARE IRRELEVENT)
		close(fd[1]);

		// Get information from the parent
		char *buffer = NULL;
		size_t n = 0;
		int output_fd = open("output.txt", O_CREAT|O_WRONLY|O_TRUNC, 0666);
		if (output_fd == -1) {
			close(fd[0]);
			return EXIT_FAILURE;
		}

		// if (fwrite("hello", sizeof(char), 5, output_fd) == -1) {
		if (write(output_fd, "hello", sizeof(char)*strlen("hello")) == -1) {
			close(output_fd);
			close(fd[0]);
			return EXIT_FAILURE;
		}

		// while ((n = getline(&buffer, &n, f)) != -1) {
		// 	// TODO write to output.txt
		// 	// printf("%s", buffer);
		// 	if (fwrite(buffer, sizeof(char), n, output_fd) == -1) {
		// 		free(buffer);
		// 		fclose(f);
		// 		fclose(output_fd);
		// 		close(fd[0]);
		// 		return EXIT_FAILURE;
		// 	}
		// }

		// Cleanup
		close(fd[0]);
		// free(buffer);
		close(output_fd);

		return EXIT_SUCCESS;
	}
	else { // parent code
		close(fd[0]); // close the read end of the file descriptor

		char message[] = "this message was sent from the parent to the child (the child wrote it to output.txt)\n";

		if (write(fd[1], message, sizeof(char)*strlen("hello")) == -1) {
			close(fd[1]);
			return EXIT_FAILURE;
		}

		// wait for the child process to finish
		waitpid(pid, &wait_status, 0);

		if (!WIFEXITED(wait_status)) {
			close(fd[1]);
			return EXIT_FAILURE;
		}

		close(fd[1]);

		printf("program finished successfully...\n");
		return EXIT_SUCCESS;
	}
}
