#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
	// TODO: change the standardout to a particular file or array or point in memory
	// output of the commands
	// char[]* pipedOutput;
	pid_t pid = 0;
	int wait_status = 0;



	// create child ONE
	pid = fork();
	if (pid == -1) { 
		return 1; // exit failure
	}

	// child ONE will execute the 'ls' command
	if (pid == 0) {
		// execute the ls command
		execvp("ls", NULL);
		return 1; // exit failure
	}

	waitpid(pid, &wait_status, 0);

	// break if something went wrong with child 1
	if (!WIFEXITED(wait_status)) { 
		return 1; // exit failure
	}

	// create child TWO
	pid = fork();
	if (pid == -1) { 
		return 1; // exit failure
	}

	// child TWO will execute the 'ls' command
	if (pid == 0) {
		execvp("ls", NULL);
		return 1; // exit failure
	}

	waitpid(pid, &wait_status, 0);

	// break if something went wrong with child 1
	if (!WIFEXITED(wait_status)) { 
		return 1; 
	}

	printf("program finished successfully...\n");
	return 0;
}
