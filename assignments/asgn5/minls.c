#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "parse.h"
#include "print.h"
#include "minls.h"
#include "zone.h"
#include "disk.h"

int main (int argc, char *argv[]) {
  /* Verbosity. If set, print the partition table(s) superblock, and inode of 
     source file/directory to stderr. */
  bool verbose = false;
  
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
  int pd_args = parse_minls_input(
      argc, 
      argv, 
      &imagefile_path, 
      &minix_path, 
      pd_flags);

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
  
  /* The address of the inode that will be populated if the path is found. */
  min_inode inode;

  /* The current name that was just processed. After the last item in the path
     was processed, this will be set to the last file/directory name.*/
  unsigned char* cur_name = malloc(sizeof(char)*DIR_NAME_SIZE+1);
  if (cur_name == NULL) {
    fprintf(stderr, "error mallocing space to track the cur name: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  /* By default, set the string to be empty. */
  *cur_name = '\0';
  
  /* Allocate enough space for the canonicalized interpretation of the given 
     minix path. */
  char* can_minix_path = malloc(sizeof(char)*strlen(minix_path)+1);
  if (can_minix_path == NULL) {
    fprintf(stderr, "error mallocing canonicalized minix path: %d\n", errno);
    exit(EXIT_FAILURE);
  }


  /* By default, set the string to be empty. */
  *can_minix_path = '\0';

  /* Try to find the path. The inode will be updated if the inode was found; 
     can_minix_path, and cur_name will be updated as the search progresses. */
  if (!find_inode(&mfs, &inode, minix_path, can_minix_path, cur_name)) {
    fprintf(stderr, "The path [%s] was not found!\n", can_minix_path);
    exit(EXIT_FAILURE);
  }

  /* Remove the trailing slash if the string is not empty. */
  if(can_minix_path[0] != '\0') {
    can_minix_path[strlen(can_minix_path)-1] = '\0';
  }

  /* Print inode information when the file is found. */
  if (verbose) {
    print_inode(stderr, &inode);
  }

  /* If we have fully traversed the path and landed on a directory, list all 
     elements in that directory. */
  if (inode.mode & DIR_FT) {
    /* The root DELIMITER is going to be added manualy. */
    ls_directory(stdout, &mfs, &inode, can_minix_path);
  }
  /* Otherwise, just list the single file we landed on. */
  else {
    /* note that when listing a single file, we don't want to print the 
       DELIMITER*/
    ls_file(stdout, &inode, (unsigned char*)can_minix_path);
  }

  /* Close the minix filesystem. */
  close_mfs(&mfs);

  /* Free the malloc'ed canonicalized path string. */
  free(can_minix_path);

  exit(EXIT_SUCCESS);
}

/* ========================================================================== */

/* Prints some information about an inode including the rwx permissions for the
 * Group, User, and Other, it's size, and it's name.
 * @param s the stream that this message will be printed to.
 * @param inode the inode of interest. 
 * @param name the name of the file.
 * @return void.
 */
void ls_file(FILE* s, min_inode* inode, unsigned char* name) {
  /* prints whether this is a directory or not */
  print_mask(s, "d", inode->mode, DIR_FT);

  /* Print the owner permissions. */
  print_mask(s, "r", inode->mode, OWNER_R_PEM);
  print_mask(s, "w", inode->mode, OWNER_W_PEM);
  print_mask(s, "x", inode->mode, OWNER_X_PEM);

  /* Primaskup permissions. */
  print_mask(s, "r", inode->mode, GROUP_R_PEM);
  print_mask(s, "w", inode->mode, GROUP_W_PEM);
  print_mask(s, "x", inode->mode, GROUP_X_PEM);

  /* Primasker permissions. */
  print_mask(s, "r", inode->mode, OTHER_R_PEM);
  print_mask(s, "w", inode->mode, OTHER_W_PEM);
  print_mask(s, "x", inode->mode, OTHER_X_PEM);

  fprintf(s, "%*u %s\n", FILE_SIZE_LN, inode->size, name);
}

/* Block processor callback for listing directory entries */
bool list_block(FILE* s, min_fs* mfs, min_inode* inode,
                uint32_t zone_num, uint32_t block_num, void* context) {
  uint32_t zone_addr = mfs->partition_start + (zone_num * mfs->zone_size);
  uint32_t block_addr = zone_addr + (block_num * mfs->sb.blocksize);

  uint32_t num_directories = mfs->sb.blocksize / DIR_ENTRY_SIZE;

  for (int i = 0; i < num_directories; i++) {
    min_dir_entry entry;

    if (fseek(mfs->file, block_addr + (i * DIR_ENTRY_SIZE), SEEK_SET)) {
      fprintf(stderr, "error seeking to directory entry: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    if (fread(&entry, DIR_ENTRY_SIZE, 1, mfs->file) < 1) {
      fprintf(stderr, "error reading directory entry: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    if (entry.inode != 0) {
      min_inode next_inode;

      if (fseek(mfs->file, 
                mfs->b_inodes + ((entry.inode - 1) * INODE_SIZE),
                SEEK_SET) == -1) {
        fprintf(stderr, "error seeking to inode: %d\n", errno);
        exit(EXIT_FAILURE);
      }

      if (fread(&next_inode, INODE_SIZE, 1, mfs->file) < 1) {
        fprintf(stderr, "error getting next inode for printing: %d\n", errno);
        exit(EXIT_FAILURE);
      }

      ls_file(s, &next_inode, entry.name);
    }
  }

  return false; /* Never stop early when listing */
}

void ls_directory(FILE* s, min_fs* mfs, min_inode* inode, char* can_path) {
  fprintf(s, "/%s:\n", can_path);

  /* Process direct zones */
  for (int i = 0; i < DIRECT_ZONES; i++) {
    process_direct_zone(s, mfs, inode, inode->zone[i], list_block, NULL, NULL);
  }

  /* Process indirect zone */
  process_indirect_zone(s, mfs, inode, inode->indirect, list_block, NULL, NULL);

  /* Process double indirect zone */
  process_two_indirect_zone(
      s, 
      mfs, 
      inode, 
      inode->two_indirect, 
      list_block, 
      NULL, 
      NULL);
}

