#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>

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

  /* The path to the imagefile (where the "disk" is on the host machine). */
  char* imagefile_path = NULL;

  /* The path to ls in minix.*/
  char* minix_path = NULL;
  
  /* Parses the flags passed into this function, setting the verbosity, primary
     partition, subpartition numbers, and returning the number of flags
     processed. If something went wrong when parsing the flags, -1 is returned,
     and the error message is printed in the function. */
  int pd_flags = parse_flags(argc, argv, &verbose, &prim_part, &sub_part);

  /* Parses the path arguments, pointing imagefile and path to the respective
     strings in argv upon success. If something went wrong when parsing the 
     arguments, -1 is returned, and the error message is printed in that 
     function. */
  int pd_args = parse_minls_input(argc, argv, &imagefile_path, &minix_path, pd_flags);

  /* Catch errors by printing the general usage statement and exiting. */
  if (pd_flags == -1 || pd_args == -1) {
    minls_usage();
    exit(EXIT_FAILURE);
  }

  if (minix_path == NULL) {
    /* Set the default path to the root directory '/' */
    minix_path = "/";
  }

  printf("\n--- PARSED ITEMS ---\n");
  printf("IMAGEFILE PATH: %s\n", imagefile_path);
  printf("MINIX PATH: %s\n", minix_path);
  printf("VERBOSE: %d\n", verbose);
  printf("PRIM PART: %d\n", prim_part);
  printf("SUB PART: %d\n\n", sub_part);

  /* load the imagefile as readonly */
  int imagefile_fd = open(imagefile_path, O_RDONLY);
  if (imagefile_fd == 0) {
    fprintf(
        stderr, 
        "could not open the imagefile at %s: %d", 
        imagefile_path, 
        errno);
    exit(EXIT_FAILURE);
  }

  
  close(imagefile_fd);
  exit(EXIT_SUCCESS);
}

