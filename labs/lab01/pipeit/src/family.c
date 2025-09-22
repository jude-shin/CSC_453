#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 2048 // write size for buffers writing to files
#define OUT_PERMS 0644 // permissions for the 'outfile'


/*
Function:
  Executes the '$ sort -r' command on the incoming stream from pipe ipc_fd
  and writes it's output to a file in the project directory called
  'outfile'. If that file is not found, a file called 'outfile' is created
  and used.
Parameters: 
  ipc_fd: A Pointer to the pipe which child1 and child2 will communicate. 
  Note that ipc_fd[0] and ipc_fd[1] represent the 'read' end 'write' end of 
  the pipe respectively.
Returns:
   void.
*/
void child2(int *ipc_fd) {
  // The "read side" of the ipc_fd pipe will be used, so close the unused 
  // "write side" of that pipe.
  close(ipc_fd[1]);

  // Make the outfile_fd file descriptor and open as writable, creating the
  // file if it does not exsist. Give general permissions to the "outfile".
  int outfile_fd = open("outfile", O_CREAT|O_WRONLY|O_TRUNC, OUT_PERMS); 
  if (outfile_fd == -1) {
    close(ipc_fd[0]);
    perror("[child2] error opening oufile");
    exit(EXIT_FAILURE);
  }

  // Change stdin to be the "read side" of the ipc_fd pipe so the "sort"  
  // command uses the output from child1's exec of the "ls" command.
  if (dup2(ipc_fd[0], STDIN_FILENO) == -1) {
    close(ipc_fd[0]);
    exit(EXIT_FAILURE);
  }

  // Change stdout to be the outfile_fd so that the "sort -r" command writes 
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
  perror("[child2] error executing \"sort -r\" command");
  exit(EXIT_FAILURE);
}




/*
Function:
  Executes the '$ ls' command and tosses that output through the pipe ipc_fd 
  for child 2.
Parameters: 
  ipc_fd: A Pointer to the pipe which child1 and child2 will communicate. 
  Note that ipc_fd[0] and ipc_fd[1] represent the 'read' end 'write' end of 
  the pipe respectively.
Returns:
  void.
*/
void child1(int *ipc_fd) {
  // The "write side" of the ipc_fd pipe will be used, so close the unused
  // "read side" of that pipe.
  close(ipc_fd[0]);

  // Change stdout to be the "write side" of the ipc_fd pipe so that the "ls"
  // command writes to the ipc_fd pipe that will be used by child2.
  if (dup2(ipc_fd[1], STDOUT_FILENO) == -1) {
    close(ipc_fd[1]); 
    exit(EXIT_FAILURE);
  }

  close(ipc_fd[1]); 

  // Note: execlp will only return on failure, and will send the appropriate 
  // exit(EXIT_FAILURE) and exit(EXIT_SUCCESS) signals to the parent.
  execlp("ls", "ls", NULL);
  perror("[child1] error executing \"ls\" command");
  exit(EXIT_FAILURE);
}
