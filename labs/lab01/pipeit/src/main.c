#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include "family.h"

// TODO: ASK Ask ask if I should close the write end here (asap), or
// in the function (as it is now... for readablity)
// for both the child1 and parent functions



int main(int argc, char *argv[]) {
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
    // perform child1 actions
    // take stdin from parent_child1_fd pipe that was written by parent
    // redirect stdout to output_fd pipe for the ouput file 
    // execute '$ sort -r' command, which writes the stream to the 
    child1(parent_child1_fd);
  }

  // perform parent actions
  // redirect stdout to parent_child1_fd pipe for child1 to read
  // execute '$ ls' command,
  parent(parent_child1_fd);
}
