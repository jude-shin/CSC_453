// make sure these are all needed
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include "helper.h"

#define BUFFER_SIZE 2048
#define OUT_PERMS 0644 // permissions for the output file

void child(int *ipc_fd) {
  // keep the "read side" of ipc_fd
  // (therefore, close the "write side" of that pipe)
  clean_close(ipc_fd[1]);

  // make the output_fd and open as (write, and create file if nessicary)
  // give general permissions to the file as well
  int output_fd = open("output", O_CREAT|O_WRONLY|O_TRUNC, OUT_PERMS); 
  if (output_fd == -1) {
    clean_close(ipc_fd[0]);
    perror("[child2] could not open output file");
    exit(EXIT_FAILURE);
  }

  // change stdin to be from the ipc_fd "read side"
  if (dup2(ipc_fd[0], STDIN_FILENO) == -1) {
    clean_close(ipc_fd[0]);
    exit(EXIT_FAILURE);
  }
  // change stdout to be to the output_fd
  if (dup2(output_fd, STDOUT_FILENO) == -1) {
    clean_close(ipc_fd[0]);
    clean_close(output_fd);
    exit(EXIT_FAILURE);
  }

  clean_close(ipc_fd[0]);
  clean_close(output_fd);

  // execute sort
  if (execlp("sort", "sort", "-r", NULL) == -1) {
    perror("[child1] error executing \"$sort -r\" command");
    exit(EXIT_FAILURE);
  }
}

void parent(int *ipc_fd) {
  // keep the "write side" of ipc_fd
  // (therefore, close the "read side" of that pipe)
  clean_close(ipc_fd[0]);

  // redirect stdout to the file descriptor that is the "write" end of 
  // the parent to child 1 pipe
  if (dup2(ipc_fd[1], STDOUT_FILENO) == -1) {
    clean_close(ipc_fd[1]); 
    exit(EXIT_FAILURE);
  }

  clean_close(ipc_fd[1]); 

  // the output which goes to stdout is now the write end of the pipe
  // when ls is executed, the output will flow through that pipe
  if (execlp("ls", "ls", NULL) == -1) {
    perror("[parent] error executing \"$ls\" command");
    exit(EXIT_FAILURE);
  }
}
