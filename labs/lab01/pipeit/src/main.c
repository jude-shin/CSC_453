#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#include "family.h"

// TODO: ASK Ask ask if I should close the write end here (asap), or
// in the function (as it is now... for readablity)
// for both the child and parent functions

int main(int argc, char *argv[]) {
  // forking variables
  int wait_status_c1, wait_status_c2;
  pid_t pid_c1, pid_c2 = 0;

  // setup file descriptors (a pipe) for c1 <-> c2 processes
  int ipc_fd[2]; 
  if (pipe(ipc_fd) == -1) {
    perror("[parent] error making pipe ipc_fd");
    exit(EXIT_FAILURE);
  }
  
  // ==========================================
  // create the child 1 process 
  pid_c1 = fork();
  if (pid_c1 == -1) { 
    close(ipc_fd[0]);
    close(ipc_fd[1]);
    perror("[parent] error forking child 1");
    exit(EXIT_FAILURE);
  }

  // perform child 1 actions
  if (pid_c1 == 0) {
    child1(ipc_fd);
  }

  // ==========================================
  // create the child 2 process 
  pid_c2 = fork();
  if (pid_c2 == -1) { 
    close(ipc_fd[0]);
    close(ipc_fd[1]);
    perror("[parent] error forking child 2");
    exit(EXIT_FAILURE);
  }
  
  // perform child 2 actions
  if (pid_c2 == 0) {
    child2(ipc_fd);
  }

  // ==========================================
  // parent code

  close(ipc_fd[0]);
  close(ipc_fd[1]);

  // wait for both processes to finish
  // if both are successfull, finish the program and return 0
  // else, something went wrong, so return -1
  waitpid(pid_c1, &wait_status_c1, 0);
  waitpid(pid_c2, &wait_status_c2, 0);

  if (WIFEXITED(wait_status_c1) && WIFEXITED(wait_status_c2)) {
    printf("Program Finished Successfully!\n");
    return 0;
  }

  return -1;
}
