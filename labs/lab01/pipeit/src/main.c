#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include "family.h"
#include "helper.h"

// TODO: ASK Ask ask if I should close the write end here (asap), or
// in the function (as it is now... for readablity)
// for both the child and parent functions

int main(int argc, char *argv[]) {
  // forking
  pid_t pid = 0;

  // file desc. for parent <-> child
  int ipc_fd[2]; 
  if (pipe(ipc_fd) == -1) {
    perror("[parent] error making pipe ipc_fd");
    exit(EXIT_FAILURE);
  }

  // create a new process (child) as parent
  pid = fork();
  if (pid == -1) { 
    clean_close(ipc_fd[0]);
    clean_close(ipc_fd[1]);
    perror("[parent] error forking child");
    exit(EXIT_FAILURE);
  }

  if (pid == 0) {
    // perform child actions
    // take stdin from ipc_fd pipe that was written by parent
    // redirect stdout to output_fd pipe for the ouput file 
    // execute '$ sort -r' command, which writes the stream to the 
    child(ipc_fd);
  }

  // perform parent actions
  // redirect stdout to ipc_fd pipe for child to read
  // execute '$ ls' command,
  parent(ipc_fd);
}
