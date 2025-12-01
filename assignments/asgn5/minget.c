#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include "input.h"
#include "print.h"
#include "disk.h"

/* TODO: for all files, check that there are. */

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

  /* The path to the src file in the minix image. */
  char* minix_src_path = NULL;

  /* The path to the dst file in the minix image. */
  char* minix_dst_path = NULL;
  
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
  int pd_args = parse_minget_input(
      argc, 
      argv, 
      &imagefile_path, 
      &minix_src_path, 
      &minix_dst_path, 
      pd_flags);

  /* Catch errors by printing the general usage statement and exiting. */
  if (pd_flags == -1 || pd_args == -1) {
    print_minget_usage(stderr);
    exit(EXIT_FAILURE);
  }

  /* ======================================================================== */

  /* Open the minix filesystem, populating the values in the mfs struct. */
  open_mfs(&mfs, imagefile_path, prim_part, sub_part, verbose);
 
  /* ======================================================================== */

  /* The inode that will be populated if the source path is found. */
  uint32_t src_inode_addr;

  if (!find_inode(&mfs, &src_inode_addr, minix_src_path, NULL, NULL)) {
    fprintf(stderr, "The path [%s] was not found!\n", minix_src_path);
    exit(EXIT_FAILURE);
  }

  /* Make a copy of the inode so I don't have to keep reading from the image. */
  min_inode src_inode;
  duplicate_inode(&mfs, src_inode_addr, &src_inode);

  if (!(src_inode.mode & REG_FT)) {
    fprintf(stderr, "The path [%s] is not a regular file!\n", minix_src_path);
    exit(EXIT_FAILURE);
  }

  /* Print inode information when the file is found. */
  if (verbose) {
    print_inode(stderr, &src_inode);
  }


  /* If there was no dst file, just print it to stdout. */
  if (minix_dst_path == NULL) {
    print_file_contents(stdout, &mfs, &src_inode);
  }
  /* If there was a dst file, try to write it there. */
  else {
    /* Get the stats of the file on the file. */
    struct stat sb;
    if (stat(minix_dst_path, &sb) == -1) {
      fprintf(stderr, "error getting stat on dst path: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Mask the mode to see if the file is regular. */
    if (!S_ISREG(sb.st_mode)) {
      fprintf(stderr, "dst is not a regular file: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Open the file for writing. */
    FILE* output = fopen(minix_dst_path, "wb");
    if (output == 0) {
      fprintf(stderr, "error opening %s: %d\n", minix_dst_path, errno);
      exit(EXIT_FAILURE);
    }

    print_file_contents(output, &mfs, &src_inode);
    fclose(output);
  }

  /* Close the minix filesystem. */
  close_mfs(&mfs);

  exit(EXIT_SUCCESS);
}

