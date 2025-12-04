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

/* Prints an error that shows the flags that can be used with minls.
 * @param s the stream that this message will be printed to.
 * @return void.
 */
void print_minls_usage(FILE* s) {
  fprintf(
      s,
      "usage: minls [-v] [-p num [-s num]] imagefile [path]\n"
      "Options:\n"
      "-p part    --- select partition for filesystem (default: none)\n"
      "-s sub     --- select subpartition for filesystem (default: none)\n"
      "-v verbose --- increase verbosity level\n"
      );
}

/* Prints some information about an inode including the rwx permissions for the
 * Group, User, and Other, it's size, and it's name.
 * @param s the stream that this message will be printed to.
 * @param inode the inode of interest. 
 * @param name the name of the file.
 * @return void.
 */
/* TODO: do we have to check that this is null terminated at 60 characters? */
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

/* TODO: print the files in a block  COMMENTS*/
void ls_files_in_block(
    FILE* s, 
    min_fs* mfs, 
    uint32_t zone_num, 
    uint32_t block_number) {
  /* Skip over the zone if it is not used. */
  if (zone_num == 0) { 
    return ;
  }

  uint32_t zone_addr = mfs->partition_start + (zone_num * mfs->zone_size);

  uint32_t block_addr = zone_addr + (block_number * mfs->sb.blocksize);


  /* Read all directory entries in this block. */
  uint32_t num_directories = mfs->sb.blocksize / DIR_ENTRY_SIZE;

  int i;
  for(i = 0; i < num_directories; i++) {
    /* print ALL dierctory entries that fit in this block. */

    /* The entry that will hold the filename. */
    min_dir_entry entry;

    /* Seek to the directory entry */
    if (fseek(mfs->file, block_addr + (i * DIR_ENTRY_SIZE), SEEK_SET)) {
      fprintf(stderr, "error seeking to directory entry: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Read the directory entry */
    if(fread(&entry, DIR_ENTRY_SIZE, 1, mfs->file) < 1) {
      fprintf(stderr, "error reading directory entry: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Check if entry is valid */
    if(entry.inode != 0) {
      min_inode next_inode;

      /* Seek to the address that holds the inode that we are on. */
      if (fseek(
            mfs->file, 
            mfs->b_inodes + ((entry.inode - 1) * INODE_SIZE),
            SEEK_SET) == -1) {
        fprintf(stderr, "error seeking to inode: %d\n", errno);
        exit(EXIT_FAILURE);
      }

      /* Fill the next_inode with the found information. */
      if(fread(&next_inode, INODE_SIZE, 1, mfs->file) < 1) {
        /* If there was an error writing to the inode, note it an exit. We 
           could try to limp along, but this is not that critical of an 
           application, so we'll just bail. */
        fprintf(stderr, "error getting next inode for printing: %d\n", errno);
        exit(EXIT_FAILURE);
      }

      ls_file(s, &next_inode, entry.name);
    }
  }
}

/* Given a zone, print all of the files that are on that zone if they are valid.
 * @param s the stream that this message will be printed to.
 * @param mfs a struct that holds an open file descriptor, and the offset (the
 *  offset from the beginning of the image which indicates the beginning of the
 *  partition that holds the filesystem.
 * @param zone_num the zone number of interest
 * @return void.
 */
void ls_files_in_direct_zone(FILE* s, min_fs* mfs, uint32_t zone_num) {
  /* Skip over the indirect zone if it is not used. */
  if (zone_num == 0) { 
    return ;
  }

  /* Read all directory entries in this chunk. */
  uint32_t num_blocks = mfs->zone_size / mfs->sb.blocksize;

  int i;
  for(i = 0; i < num_blocks; i++) {
    /* print ALL blocks that fit in this zone. */
    ls_files_in_block(s, mfs, zone_num, i);
  }
}

/* Given an indirect zone, print all of the files that are on the zones that
 * the indirect zone points to if they are valid.
 * @param s the stream that this message will be printed to.
 * @param mfs a struct that holds an open file descriptor, and the offset (the
 *  offset from the beginning of the image which indicates the beginning of the
 *  partition that holds the filesystem.
 * @param zone_num the indirect zone number of interest
 * @return void.
 */
void ls_files_in_indirect_zone(FILE* s, min_fs* mfs, uint32_t zone_num) {
  /* Skip over the indirect zone if it is not used. */
  if (zone_num == 0) {
    return;
  }

  /* The first block is going to line up with the address of that zone. */
  uint32_t block_addr = mfs->partition_start + (zone_num*mfs->zone_size);

  /* How many zone numbers we are going to read (how many fit in the first 
     block of the indirect zone) */
  uint32_t num_indirect_inodes = mfs->sb.blocksize / sizeof(uint32_t);

  /* For every zone number in that first indirect inode block. */
  int i;
  for(i = 0; i < num_indirect_inodes; i++) {
    /* The zone number that holds directory entries. */
    uint32_t indirect_zone_number;

    /* Read the entry offset in the first block. */
    if (fseek(
          mfs->file, 
          block_addr + (i * sizeof(uint32_t)),
          SEEK_SET) == -1) {
      fprintf(stderr, "error seeking to indirect zone: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Read the number that holds the zone number. */
    if(fread(&indirect_zone_number, sizeof(uint32_t), 1, mfs->file) < 1) {
      fprintf(stderr, "error reading indirect zone: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Print all the valid files that are in the first block in this list. */
    ls_files_in_direct_zone(s, mfs, indirect_zone_number);
  }
}

/* Given a double indirect zone, print all of the files that are on the zones 
 * that are on the zones that the indirect zone points to if they are valid
 * (double indirect -> indirect -> zone -> file).
 * @param s the stream that this message will be printed to.
 * @param mfs a struct that holds an open file descriptor, and the offset (the
 *  offset from the beginning of the image which indicates the beginning of the
 *  partition that holds the filesystem.
 * @param zone_num the double indirect zone number of interest
 * @return void.
 */
void ls_files_in_two_indirect_zone(FILE* s, min_fs* mfs, uint32_t zone_num) {
  /* Skip over the indirect zone if it is not used. */
  if (zone_num == 0) {
    return;
  }

  /* How many zone numbers we are going to read (how many fit in the first 
     block of the indirect zone) */
  uint32_t num_indirect_inodes = mfs->sb.blocksize / sizeof(uint32_t);

  uint32_t zone_addr = mfs->partition_start + (zone_num*mfs->zone_size);

  /* For every zone number in that first double indirect inode block. */
  int i;
  for(i = 0; i < num_indirect_inodes; i++) {
    /* The zone number that holds the indierct zone numbers. */
    uint32_t indirect_zone_number;

    /* Start reading the first block in the double indirect zone. */
    if (fseek(
          mfs->file, 
          zone_addr + (i * sizeof(uint32_t)), 
          SEEK_SET) == -1) {
      fprintf(stderr, "error seeking to double indirect zone: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Read the number that holds the zone number. */
    if(fread(&indirect_zone_number, sizeof(uint32_t), 1, mfs->file) < 1) {
      fprintf(stderr, "error reading indirect zone number: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Print the files in that indirect zone. */
    ls_files_in_indirect_zone(s, mfs, indirect_zone_number);
  }
}

/* Prints every directory entry in a directory.
 * @param s the stream that this message will be printed to.
 * @param mfs a struct that holds an open file descriptor, and the offset (the
 *  offset from the beginning of the image which indicates the beginning of the
 *  partition that holds the filesystem.
 * @param inode the inode of interest. 
 * @param can_path the canonicalized path that we are going to list.
 * @return void.
 */
void ls_directory(FILE* s, min_fs* mfs, min_inode* inode, char* can_path) {
  /* print the full canonicalized minix path that we are listing. */
  fprintf(s, "/%s:\n", can_path);

  /* DIRECT ZONES */
  int i;
  for(i = 0; i < DIRECT_ZONES; i++) {
    /* Print all of the valid files that are in that zone. */
    ls_files_in_direct_zone(s, mfs, inode->zone[i]);
  }

  /* INDIRECT ZONES */
  ls_files_in_indirect_zone(s, mfs, inode->indirect);

  /* DOUBLE INDIRECT ZONES */
  ls_files_in_two_indirect_zone(s, mfs, inode->two_indirect);
}


