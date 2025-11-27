#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "input.h"
#include "messages.h"

int main (int argc, char *argv[]) {
  /* Verbosity. If set, print the partition table(s) superblock, and inode of 
     source file/directory to stderr. */
  int verbose = false;
  
  /* What primarty partition number to use (-1 means unpartitioned. )*/
  int prim_part = -1;

  /* What subpartition number to use (-1 means unpartitioned. ) */
  int sub_part = -1;

  /* The image file argument given. */
  char* imagefile = NULL;

  /* The path argument given. */
  char* path = NULL;
  
  /* Parses the flags passed into this function, setting the verbosity, primary
     partition, subpartition numbers, and returning the number of flags
     processed. If something went wrong when parsing the flags, -1 is returned,
     and the error message is printed in the function. */
  int pd_flags = parse_flags(argc, argv, &verbose, &prim_part, &sub_part);

  /* Parses the path arguments, pointing imagefile and path to the respective
     strings in argv upon success. If something went wrong when parsing the 
     arguments, -1 is returned, and the error message is printed in that 
     function. */
  int pd_args = parse_minls_input(argc, argv, &imagefile, &path, pd_flags);

  /* Catch errors by printing the general usage statement and exiting. */
  if (pd_flags == -1 || pd_args == -1) {
    minls_usage();
    exit(EXIT_FAILURE);
  }

  if (path == NULL) {
    /* Set the default path to the root directory '/' */
    path = "/";
  }

  printf("\n--- PARSED ITEMS ---\n");
  printf("IMAGEFILE: %s\n", imagefile);
  printf("PATH: %s\n", path);
  printf("VERBOSE: %d\n", verbose);
  printf("PRIM PART: %d\n", prim_part);
  printf("SUB PART: %d\n\n", sub_part);

  exit(EXIT_SUCCESS);
}

