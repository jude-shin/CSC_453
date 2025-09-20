#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include "family.h"

int main(int argc, char *argv[]) {
  // hellllo this is new 
  // forking
  pid_t pid = 0;

  // file desc. for parent -> child1
  int parent_child1_fd[2]; 
  if (pipe(parent_child1_fd) == -1) {
    perror("[parent] error making pipe parent_child1_fd");
    return EXIT_FAILURE;
  }

  // create a new process (child 1) as parent
  pid = fork();
  if (pid == -1) { 
    close(parent_child1_fd[0]);
    close(parent_child1_fd[1]);
    perror("[parent] error forking child1");
    return EXIT_FAILURE;
  }

  if (pid == 0) {
    // file desc. for child1 -> child2 pipe 
    int child1_child2_fd[2]; 
    if (pipe(child1_child2_fd) == -1) {
      perror("[child1] error making pipe child1_child2_fd");
      return EXIT_FAILURE;
    }

    // create a new process (child2) as child1
    pid = fork();
    if (pid == -1) { 
      close(parent_child1_fd[0]);
      close(parent_child1_fd[1]);
      close(child1_child2_fd[0]);
      close(child1_child2_fd[1]);
      perror("[child1] error forking child2");
      return EXIT_FAILURE;
    }
    
    if (pid == 0)	{ // if you are child2:
      // we don't need to access pipe parent_child1_fd as child2
      close(parent_child1_fd[0]);
      close(parent_child1_fd[1]);

      // TODO: ASK Ask ask if I should close the write end here (asap), or
      // in the function (as it is now... for readablity)

      // redirect stdout to a new file descriptor for the output file
      // take stream and write to the output fd using a buffer
      child2(child1_child2_fd);
    }
    
    // perform child1 actions
    // take stdin from parent_child1_fd pipe that was written by parent
    // redirect stdout to child1_child2_fd pipe for child2 to read
    // execute '$ sort -r' command
    child1(parent_child1_fd, child1_child2_fd);
  }
  
  // TODO: ASK Ask ask if I should close the write end here (asap), or
  // in the function (as it is now... for readablity)

  // perform parent actions
  // redirect stdout to parent_child1_fd pipe for child1 to read
  // execute '$ ls' command,
  parent(parent_child1_fd);
}

