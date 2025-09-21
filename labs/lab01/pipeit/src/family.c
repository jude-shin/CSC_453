#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 2048
#define OUT_PERMS 0644 // permissions for the output file

void child2(int *ipc_fd) {
  // The "read side" of the ipc_fd pipe will be used, so close the unused 
  // "write side" of that pipe.
  close(ipc_fd[1]);

  // Make the outfile_fd file descriptor and open as writable, creating the
  // file if it does not exsist. Give general permissions to it as well.
  int outfile_fd = open("outfile", O_CREAT|O_WRONLY|O_TRUNC, OUT_PERMS); 
  if (outfile_fd == -1) {
    close(ipc_fd[0]);
    perror("[child2] could not open output file");
    exit(EXIT_FAILURE);
  }

  // Change stdin to be the "read side" of the ipc_fd pipe so the "sort"  
  // command uses the output from child1's exec of the "$ ls" command.
  if (dup2(ipc_fd[0], STDIN_FILENO) == -1) {
    close(ipc_fd[0]);
    exit(EXIT_FAILURE);
  }

  // Change stdout to be the outfile_fd so that the "$ sort -r" command outputs
  // to the 'outfile'.
  if (dup2(outfile_fd, STDOUT_FILENO) == -1) {
    close(ipc_fd[0]);
    close(outfile_fd);
    exit(EXIT_FAILURE);
  }

  close(ipc_fd[0]);
  close(outfile_fd);
  
  // Note: execlp will only return on failure, and will send the appropriate 
  // exit(EXIT_FAILURE) and exit(EXIT_SUCCESS) signals to the parent.
  execlp("sort", "sort", "-r", NULL);
  perror("[child2] error executing \"$sort -r\" command");
  exit(EXIT_FAILURE);
}

void child1(int *ipc_fd) {
  // The "write side" of the ipc_fd pipe will be used, so close the unused
  // "read side" of that pipe.
  close(ipc_fd[0]);

  // Change stdout to be the "write side" of the ipc_fd pipe so that the "$ ls"
  // command outputs to the ipc_fd pipe that will be used by child2.
  if (dup2(ipc_fd[1], STDOUT_FILENO) == -1) {
    close(ipc_fd[1]); 
    exit(EXIT_FAILURE);
  }

  close(ipc_fd[1]); 

  // Note: execlp will only return on failure, and will send the appropriate 
  // exit(EXIT_FAILURE) and exit(EXIT_SUCCESS) signals to the parent.
  execlp("ls", "ls", NULL);
  perror("[child1] error executing \"$ls\" command");
  exit(EXIT_FAILURE);
}
