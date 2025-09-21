#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include <wait.h>

#include "family.h"
#include "helper.h"

// TODO: ASK Ask ask if I should close the write end here (asap), or
// in the function (as it is now... for readablity)
// for both the child and parent functions

/*
   the parent should create one pipe for between child 1 and 2

   the parent should create two child processes
   the first one should perform the ls command
   the second one should perform sort -r and output logic

   the parent should be waiting for both the processs to finish

   the parent should do nothing but wait for the processes to finish
*/

int main(int argc, char *argv[]) {
  // forking
  int wait_status_c1;
  pid_t pid_c1 = 0;
  int wait_status_c2;
  pid_t pid_c2 = 0;

  // file desc. for c1 <-> c2 
  int ipc_fd[2]; 
  if (pipe(ipc_fd) == -1) {
    perror("[parent] error making pipe ipc_fd");
    exit(EXIT_FAILURE);
  }
  
  // ==========================================
  // create the child 1 process 
  pid_c1 = fork();
  if (pid_c1 == -1) { 
    clean_close(ipc_fd[0]);
    clean_close(ipc_fd[1]);
    perror("[parent] error forking child 1");
    exit(EXIT_FAILURE);
  }

  if (pid_c1 == 0) {
    // perform child actions
    // take stdin from ipc_fd pipe that was written by parent
    // redirect stdout to output_fd pipe for the ouput file 
    // execute '$ sort -r' command, which writes the stream to the 
    parent(ipc_fd);
  }

  // ==========================================
  // create the child 2 process 
  pid_c2 = fork();
  if (pid_c2 == -1) { 
    clean_close(ipc_fd[0]);
    clean_close(ipc_fd[1]);
    perror("[parent] error forking child 2");
    exit(EXIT_FAILURE);
  }

  if (pid_c2 == 0) {
    child(ipc_fd);
  }

  // ==========================================
  // parent code
  // wait for both processes to finish
  // if both are successfull, then return 0, else, return -1

    waitpid(pid_c1, &wait_status_c1, 0);
    waitpid(pid_c2, &wait_status_c1, 0);

    if (WIFEXITED(wait_status_c1) && WIFEXITED(wait_status_c2)) {
        printf("Program Finished Successfully\n");
        return 0;
    }
    return -1;
}
