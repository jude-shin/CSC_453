#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "input.h"
#include "print.h"
#include "disk.h"

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
  
  /* A struct that represents the minix filesystem. */
  min_fs mfs;

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
    print_minls_usage(stderr);
    exit(EXIT_FAILURE);
  }

  if (minix_path == NULL) {
    /* Set the default path to the root directory '/' */
    minix_path = DELIMITER;
  }

  /* ======================================================================== */

  /* Open the minix filesystem, populating the values in the mfs struct. */
  open_mfs(&mfs, imagefile_path, prim_part, sub_part, verbose);
  
  /* The inode that will be populated if the path is found. */
  min_inode inode;

  /* The current name that was just processed. After the last item in the path
     was processed, this will be set to the last file/directory name.*/
  /* TODO: test with the maxed out name size. */
  unsigned char* cur_name = malloc(sizeof(char)*DIR_NAME_SIZE+1);
  /* By default, set the string to be empty. */
  *cur_name = '\0';
  
  /* Allocate enough space for the canonicalized interpretation of the given 
     minix path. */
  char* can_minix_path = malloc(sizeof(char)*strlen(minix_path)+1);

  /* By default, set the string to be empty. */
  *can_minix_path = '\0';

  /* Try to find the path. The inode will be updated if the inode was found; 
     can_minix_path, and cur_name will be updated as the search progresses. */
  if (!find_inode(&mfs, &inode, minix_path, can_minix_path, cur_name)) {
    fprintf(stderr, "The [%s] was not found!", minix_path);
    exit(EXIT_FAILURE);
  }

  /* If the canonical path is still null, then we did'nt add anything to it, 
     so it must be a variant of "/" or "//////" or some other form of the root.
     For looks, set the canonical path to the DELIMITER. */
  if (*can_minix_path == '\0') {
    strcpy(can_minix_path, DELIMITER);
  }

  /* Print inode information when the file is found. */
  if (verbose) {
    print_inode(stderr, &inode);
  }

  /* If we have fully traversed the path, but we ended up at a file, we cannot
     ls on that... we must ls on a directory. */
  if (inode.mode & DIR_FT) {
    print_directory(stderr, &mfs, &inode, can_minix_path);
    exit(EXIT_FAILURE);
  }
  else {
    print_file(stderr, &inode, cur_name);
    exit(EXIT_FAILURE);
  }


  /* Close the minix filesystem. */
  close_mfs(&mfs);

  /* Free the malloc'ed canonicalized path string. */
  free(can_minix_path);

  exit(EXIT_SUCCESS);
}

