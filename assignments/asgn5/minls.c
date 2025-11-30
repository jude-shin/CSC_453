#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "input.h"
#include "messages.h"
#include "disk.h"


void print_file() {
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
    minls_usage(stderr);
    exit(EXIT_FAILURE);
  }

  if (minix_path == NULL) {
    /* Set the default path to the root directory '/' */
    minix_path = "/foo";
  }

  printf("\n--- PARSED ITEMS ---\n");
  printf("IMAGEFILE PATH: %s\n", imagefile_path);
  printf("MINIX PATH: %s\n", minix_path);
  printf("VERBOSE: %d\n", verbose);
  printf("PRIM PART: %d\n", prim_part);
  printf("SUB PART: %d\n\n", sub_part);

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

  char* token = strtok(minix_path, "/");
  
  /* Parse all of the directories that the user gave by traversing through the
     directories till we are at the last inode. */
  while(token != NULL) {
    print_inode(mfs.file, &inode);

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


    printf("searching direct zones\n");

    /* Search the dierct zones for the current inode (directory) */
    if (search_direct_zones(&mfs, &inode, &next_inode, token)) {
      token = strtok(NULL, "/");
      inode = next_inode;
      continue;
    }

    printf("searching indirect zones\n");

    /* Search the indierct zones for the current inode (directory) */
    if (search_indirect_zones(&mfs, &inode, &next_inode, token)) {
      token = strtok(NULL, "/");
      inode = next_inode;
      continue;
    }

    printf("searching two indirect zones\n");


    /* Search the double indirect zones for the current inode (directory) */
    if (search_two_indirect_zones(&mfs, &inode, &next_inode, token)) {
      token = strtok(NULL, "/");
      inode = next_inode;
      continue;
    }

    /* Search the indirect zones by looping though all of the inodes that fit
       on the current "indirect zone". For each of those inodes, search THEIR
       direct zone.*/

    /* Go to the first block of the indirect zones */

    /* From that block*/
    /* TODO: Linear serach the double indirect zones. */

    fprintf(stderr, "error traversing the path: directory not found!\n");
    exit(EXIT_FAILURE);
  }

  /* If we have fully traversed the path, but we ended up at a file, we cannot
     ls on that... we must ls on a directory. */
  if (!(inode.mode & DIR_FT)) {
    fprintf(stderr, "error: the given path is not a directory!\n");
    exit(EXIT_FAILURE);
  }

  /* Print all the entries information in the inode directory. */





  




  /* Close the minix filesystem. */
  close_mfs(&mfs);

  exit(EXIT_SUCCESS);
}

/* ======= */
/* HELPERS */
/* ======= */
/* TODO: move the print_file and print_directotry to this section and add the
   header file after you have mucked around. */



