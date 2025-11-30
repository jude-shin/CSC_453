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
  
  /* Navigate to the root of the filesystem */
  min_inode inode;
  
  /* Seek the read head to the first inode. */
  fseek(mfs.file, mfs.b_inodes, SEEK_SET);

  /* Read the value at that address into the root inode struct. */
  if (fread(&inode, sizeof(min_inode), 1, mfs.file) < 1) {
    fprintf(stderr, "error reading the root inode: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  /* The tokenized next directory entry name that we are looking for. */
  char* token = strtok(minix_path, DELIMITER);

  /* The current name that was just processed. */
  unsigned char curr_name[DIR_NAME_SIZE] = "";

  /* Allocate enough space for the canonicalized interpretation of the given 
     minix path. */
  char* can_minix_path = malloc(sizeof(char)*strlen(minix_path)+1);
  /* By default, set the string to be the root. */
  strcpy(can_minix_path, DELIMITER);
  
  /* Parse all of the directories that the user gave by traversing through the
     directories till we are at the last inode. */
  while(token != NULL) {
    /* copy the string name to curr_name so we can keep track of the last
       processed name. */
    memcpy(curr_name, token, sizeof(char)*DIR_NAME_SIZE);

    /* Add the token to the built canonicalized minix path. */
    strcat(can_minix_path, DELIMITER);
    strcat(can_minix_path, token);

    /* The current inode must be traversable (a directory) */
    if (!(inode.mode & DIR_FT)) {
      fprintf(
          stderr, 
          "error traversing the path. %s is not a directory!\n", 
          token);
      exit(EXIT_FAILURE);
    }
   
    /* The found inode that is populated if any of the search functions find
       an inode with a matching name. */
    min_inode next_inode;

    /* Search through the direct, indirect, and double indirect zones for a 
       directory entry with a matching name. */
    if (search_all_zones(&mfs, &inode, &next_inode, token)) {
      token = strtok(NULL, DELIMITER);
      inode = next_inode;
    }
    else {
      fprintf(stderr, "error traversing the path: directory not found!\n");
      exit(EXIT_FAILURE);
    }
  }

  /* Print inode information when the file is found. */
  if (verbose) {
    print_inode(mfs.file, &inode);
  }

  /* If we have fully traversed the path, but we ended up at a file, we cannot
     ls on that... we must ls on a directory. */
  if (inode.mode & DIR_FT) {
    print_directory(stderr, &mfs, &inode, can_minix_path);
    exit(EXIT_FAILURE);
  }
  else if (inode.mode & REG_FT) {
    print_file(stderr, &inode, curr_name);
    exit(EXIT_FAILURE);
  }
  else {
    fprintf(stderr, "file selected is not a regular file or directory!\n");
    exit(EXIT_FAILURE);
  }


  /* Close the minix filesystem. */
  close_mfs(&mfs);

  /* Free the malloc'ed canonicalized path string. */
  free(can_minix_path);

  exit(EXIT_SUCCESS);
}

