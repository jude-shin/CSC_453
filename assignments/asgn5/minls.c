#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>

#include "utils.h"
#include "messages.h"

int main (int argc, char *argv[]) {
  /* Verbosity. The higher the number, the more this program talks. */
  int verbosity = 0;
  
  /* What primarty partition number to use (-1 means unpartitioned. )*/
  int prim_part = -1;

  /* What subpartition number to use (-1 means unpartitioned. ) */
  int sub_part = -1;

  /* The image file argument given. */
  char* imagefile = NULL;

  /* The path argument given. */
  char* path = NULL;

  /* Parse the inputs. */
  /* Parses the flags passed into this function, setting the verbosity, primary
     partition, subpartition numbers, and returning the number of flags
     processed. */
  int parsed_flags = parse_flags(argc, argv, &verbosity, &prim_part, &sub_part);
  /* If something went wrong in parse_flags, -1 is returned, and the error 
     message is printed in the function. Catch this case by printing the 
     general usage statement and exiting. */
  if (parsed_flags == -1) {
    minls_usage();
    exit(EXIT_FAILURE);
  }

  /* Parses the path arguments, malloc'ing space for the imagefile and path 
     upon success. Upon failure, the respective pointers are cleaned up
     (freed) already.*/
  int parsed_args = parse_minls_input(argc, argv, &imagefile, &path);
  /* If something went wrong when parsing the arguments, -1 is returned, and 
     the error message is printed in that function. Catch this case by printing
     the general usage statement and exiting. */
  if (parsed_args == -1) {
    minls_usage();
    exit(EXIT_FAILURE);
  }

  printf("\n--- PARSED ITEMS ---\n");
  printf("IMAGEFILE: %s\n", imagefile);
  printf("PATH: %s\n", path);
  printf("VERBOSITY: %d\n", verbosity);
  printf("PRIM PART: %d\n", prim_part);
  printf("SUB PART: %d\n\n", sub_part);

  exit(EXIT_SUCCESS);
}

