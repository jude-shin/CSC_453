#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>

#include "input.h"
#include "messages.h"
#include "disk.h"


void print_file(min_fs* mfs) {
  /* print the first inode (the root) */
  min_inode inode;
  
  /* Bytes read. */
  ssize_t bytes;
  
  /* Seek the read head to the first inode. */
  fseek(mfs->file, mfs->b_inodes, SEEK_SET);

  /* Read the value at that address. */
  bytes = fread(&inode, sizeof(inode), 1, mfs->file);
  if (bytes < 1) {
    fprintf(stderr, "error reading signature 1: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  print_inode(&inode);
}

void print_directory() {

}

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

  /* ======================================================================== */

  /* Open the minix filesystem, populating the values in the min_fs struct. */
  open_mfs(&mfs, imagefile_path, prim_part, sub_part, verbose);
  
  print_file(&mfs);

  /* Close the minix filesystem. */
  close_mfs(&mfs);


  exit(EXIT_SUCCESS);
}

/* ======= */
/* HELPERS */
/* ======= */
/* TODO: move the print_file and print_directotry to this section and add the
   header file after you have mucked around. */



