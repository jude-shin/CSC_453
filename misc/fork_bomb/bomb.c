#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_FORKS 5

int main(int argc, char *argv[]) {
  int i, j;
  pid_t pids[NUM_FORKS];
  pid_t pid;
  
  /* Create a bunch of dumb children */
  for (i=0; i<NUM_FORKS; i++) {
    printf("Forking Child %d\n", i);
    pid = fork();
    if (pid < 0) {
      perror("[main] error creating child");

      /* clean up all of the forks that were previously created */
      for (j=0; j < i; j++) {
        waitpid(pids[j], NULL, 0);
      }
      exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
      /* Child Code */
      printf("Child %d created!\n", i);
      /* sleep(1); */
      exit(EXIT_SUCCESS);
    }
    else {
      /* Parent Code */
      /* add the pid to the list of pids */
      pids[i] = pid;
    }
  }

  for (i=0; i<NUM_FORKS; i++) {
    printf("Waiting for child %d...\n", i);
    waitpid(pids[i], NULL, 0);
  }

  exit(EXIT_SUCCESS);
}
