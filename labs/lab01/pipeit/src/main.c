#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#include "family.h"

// TODO: Ask if I should close the write end here (asap), or
// in the function (as it is now... for readablity)
// for both the child and parent functions

// TODO: Ask if the // ========================== are annoying to the proff
// TODO: Ask about params and return in function comments
// TODO: Ask where the comments for the functions should be (in the
// .h files, or in the .c files, or both)
// TODO: Ask if I am closing everything correctly (pipes and fd)
// TODO: Ask if I am waiting correctly
// TODO: Should the child functions have the return type of void?
// TODO: Should I use the _ or camel case?

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
  
  // create the child 1 process 
  pid_c1 = fork();
  if (pid_c1 == -1) { 
    close(ipc_fd[0]);
    close(ipc_fd[1]);
    perror("[parent] error forking child 1");
    exit(EXIT_FAILURE);
  }

  // ==========================================
  // perform child 1 logic 
  if (pid_c1 == 0) {
    // exec the '$ ls' command
    // toss that output through the pipe ipc_fd for child 2 to recieve
    child1(ipc_fd);
  }

  // create the child 2 process 
  pid_c2 = fork();
  if (pid_c2 == -1) { 
    close(ipc_fd[0]);
    close(ipc_fd[1]);
    perror("[parent] error forking child 2");
    exit(EXIT_FAILURE);
  }
 
  // ==========================================
  // perform child 2 logic 
  if (pid_c2 == 0) {
    // executes the '$ sort -r' command on the incoming stream from pipe ipc_fd
    // and writes it to the file in the project directory called 'output'
    child2(ipc_fd);
  }

  // ==========================================
  // perform parent logic

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
