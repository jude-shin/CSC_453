#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include "parse.h"
#include "print.h"
#include "minget.h"
#include "zone.h"
#include "disk.h"


/* Context for file extraction */
typedef struct {
  uint32_t bytes_read;
} extract_context_t;

/* Forward declarations */
bool extract_block(FILE* s, min_fs* mfs, min_inode* inode,
                   uint32_t zone_num, uint32_t block_num, void* context);
bool handle_direct_zone_with_holes(FILE* s, min_fs* mfs, min_inode* inode,
                                    uint32_t zone_num, extract_context_t* ctx);
bool handle_indirect_zone_with_holes(FILE* s, min_fs* mfs, min_inode* inode,
                                      uint32_t zone_num, extract_context_t* ctx);
bool handle_two_indirect_zone_with_holes(FILE* s, min_fs* mfs, min_inode* inode,
                                          uint32_t zone_num, extract_context_t* ctx);


/* TODO: for all files, check that there are. */

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
  min_inode src_inode;

  if (!find_inode(&mfs, &src_inode, minix_src_path, NULL, NULL)) {
    fprintf(stderr, "The path [%s] was not found!\n", minix_src_path);
    exit(EXIT_FAILURE);
  }

  /* Check that this is a regular file. */
  if ((src_inode.mode & FILE_TYPE) != REG_FT) {
    fprintf(stderr, "The path [%s] is not a regular file!\n", minix_src_path);
    exit(EXIT_FAILURE);
  }

  /* Print inode information when the file is found. */
  if (verbose) {
    print_inode(stderr, &src_inode);
  }

  /* If there was no dst file, just print it to stdout. */
  if (minix_dst_path == NULL) {
    get_file_contents(stdout, &mfs, &src_inode);
  }

  /* If there was a dst file, try to write it there. */
  else {
    FILE *output = fopen(minix_dst_path, "wb");
    if (!output) {
      perror("error opening dst path.\n");
      exit(EXIT_FAILURE);
    }

    struct stat sb;
    if (fstat(fileno(output), &sb) == -1) {
      perror("error fstatat on dst path.\n");
      fclose(output);
      exit(EXIT_FAILURE);
    }

    if (!S_ISREG(sb.st_mode)) {
      fprintf(stderr, "dst is not a regular file\n");
      fclose(output);
      exit(EXIT_FAILURE);
    }

    get_file_contents(output, &mfs, &src_inode);
    fclose(output);
  }

  /* Close the minix filesystem. */
  close_mfs(&mfs);

  exit(EXIT_SUCCESS);
}

/* ========================================================================== */

/* Prints the contents of a regular file to the stream s given an inode. 
 * @param s the stream that this message will be printed to.
 * @param inode the inode of interest. 
 * @return void.
 */
void get_file_contents(FILE* s, min_fs* mfs, min_inode* inode) {
  uint32_t bytes_read = 0;

  /* Go through all of the direct zones sequentially. */
  int i;
  for (i = 0; i < DIRECT_ZONES; i++) {
    uint32_t zone_num = inode->zone[i];
    if (process_direct_zone(s, mfs, inode, inode->zone[i], extract_block, &bytes_read)) {
      return;
    }
  }

  /* Go through all of the indirect zones sequentially. */
  if (process_indirect_zone(
        s, 
        mfs, 
        inode, 
        inode->indirect, 
        extract_block,
        &bytes_read)) {
    return;
  }

  /* Go through all of the double indirect zones sequentially. */
  if (process_two_indirect_zone(
        s, 
        mfs, 
        inode, 
        inode->two_indirect, 
        extract_block,
        &bytes_read)) {
    return;
  }
}

bool extract_block(FILE* s, min_fs* mfs, min_inode* inode,
                   uint32_t zone_num, uint32_t block_num, void* context) {
    uint32_t* bytes_read = (uint32_t*)context;
    return get_block_contents(s, mfs, inode, zone_num, block_num, bytes_read);
}

bool fill_hole(
    FILE* s, 
    min_inode* inode, 
    uint32_t hs, /* The size of a hole */
    uint32_t* bytes_read) {

  uint32_t remaining = inode->size - *bytes_read;
  if (remaining < hs) {
    hs = remaining;
  }

  char* zeros = calloc(hs, sizeof(char));
  if (zeros == NULL) {
    fprintf(stderr, "error callocing buffer of zeros: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  /* Write a bunch of zeros. */
  if (fwrite(zeros, sizeof(char), hs, s) < hs) {
    fprintf(stderr, "error filling hole: %d\n", errno);
    free(zeros);
    exit(EXIT_FAILURE);
  }

  free(zeros);

  /* Update the bytes read. We know that since this is a hole, then this must
     be less than the total bytes read, and therefore will not go over. */
  *bytes_read = *bytes_read + hs;

  return (*bytes_read >= inode->size);
}


bool get_block_contents(
    FILE* s, 
    min_fs* mfs, 
    min_inode* inode, 
    uint32_t zone_num, 
    uint32_t block_number, 
    uint32_t* bytes_read) {
  /* TODO: I don't think we need to check the zone number... */

  uint32_t zone_addr = mfs->partition_start + (zone_num * mfs->zone_size);

  uint32_t block_addr = zone_addr + (block_number * mfs->sb.blocksize);

  /* Seek to the beginning of the block. */
  if (fseek(
        mfs->file, 
        block_addr,
        SEEK_SET) == -1) {
    fprintf(stderr, "error seeking to directory entry: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  /* Read a single character at a time so we don't have to bother with large
     varying sized buffers. */

  /* How much we have left to get. */
  uint32_t difference = inode->size - *bytes_read;

  if (difference > mfs->sb.blocksize) {
    /* If there is more to read than the size of a block, then read an entire
       blocks worth */
    difference = mfs->sb.blocksize;
  }

  /* Make a buffer to fread into. */
  void* buff = malloc(sizeof(char)*difference);
  if (buff == NULL) {
    fprintf(stderr, "error mallocing buffer to get file contents: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  /* Read the next character (byte sized). */
  if (fread(buff, sizeof(char)*difference, 1, mfs->file) < 1) {
    fprintf(stderr, "error reading character in zone: %d\n", errno);
    free(buff);
    exit(EXIT_FAILURE);
  }

  /* Write it to the desired stream. */
  fwrite(buff, sizeof(char)*difference, 1, s);

  /* Update the bytes_read count. */
  *bytes_read = *bytes_read + difference;

  free(buff);

  /* If we read everything, make note of it, and return with true. */
  return (*bytes_read >= inode->size);
}
// 
// /* Prints the contents of a zone to a stream s.  
//  * @param s the stream that this message will be printed to.
//  * @param inode the inode of interest. 
//  * @param zone_num the zone of interest.  
//  * @param how many bytes we have read so far.  
//  * @return bool if we have finished writing everything.
//  */
// bool get_direct_zone_contents(
//     FILE* s, 
//     min_fs* mfs, 
//     min_inode* inode, 
//     uint32_t zone_num, 
//     uint32_t* bytes_read) {
// 
//   /* We encountered a hole... */
//   if (zone_num == 0) {
//     /* Keep the hole still takes up space, just don't read any of it as it will
//        just be zeros. */
//     uint32_t hs = mfs->zone_size;
// 
//     return fill_hole(s, inode, hs, bytes_read);
//   }
// 
//   /* get all of the blocks in this zone. */
//   uint32_t num_blocks = mfs->zone_size / mfs->sb.blocksize;
// 
//   int i;
//   for (i = 0; i < num_blocks; i++) {
//     if (get_block_contents(s, mfs, inode, zone_num, i, bytes_read)) {
//       return true;
//     }
//   }
// 
//   /* We have not yet finished reading. */
//   return false;
// }
// 
// /* Prints the contents of an indirect zone to a stream s.  
//  * @param s the stream that this message will be printed to.
//  * @param inode the inode of interest. 
//  * @param zone_num the zone of interest.  
//  * @param how many bytes we have read so far.  
//  * @return bool if we have finished writing everything.
//  */
// bool get_indirect_zone_contents(
//     FILE* s, 
//     min_fs* mfs, 
//     min_inode* inode, 
//     uint32_t zone_num, 
//     uint32_t* bytes_read) {
// 
//   /* How many zone numbers we are going to read (how many fit in the first 
//      block of the indirect zone) */
//   uint32_t num_indirect_inodes = mfs->sb.blocksize / sizeof(uint32_t);
// 
//   /* We encountered a hole... */
//   if (zone_num == 0) {
//     uint32_t hs = mfs->sb.blocksize*num_indirect_inodes;
// 
//     return fill_hole(s, inode, hs, bytes_read);
//   }
// 
//   /* Start reading the first block in that indirect zone. */
//   if (fseek(
//         mfs->file, 
//         mfs->partition_start+(zone_num*mfs->zone_size), 
//         SEEK_SET) == -1) {
//     fprintf(stderr, "error seeking to indirect zone.\n");
//     exit(EXIT_FAILURE);
//   }
// 
//   /* The first block is going to line up with the address of that zone. */
//   uint32_t block_addr = mfs->partition_start + (zone_num*mfs->zone_size);
// 
//   /* For every zone number in that first indirect inode block. */
//   int i;
//   for(i = 0; i < num_indirect_inodes; i++) {
//     /* The zone number that holds directory entries. */
//     uint32_t indirect_zone_number;
// 
//     /* Read the entry offset in the first block. */
//     if (fseek(
//           mfs->file, 
//           block_addr + (i * sizeof(uint32_t)),
//           SEEK_SET) == -1) {
//       fprintf(stderr, "error seeking to indirect zone: %d\n", errno);
//       exit(EXIT_FAILURE);
//     }
// 
//     if (fread(&indirect_zone_number, sizeof(uint32_t), 1, mfs->file) < 1) {
//       fprintf(stderr, "error reading indirect zone: %d\n", errno);
//       exit(EXIT_FAILURE);
//     }
// 
//     /* Check to see if this is a hole. */
//     if (indirect_zone_number == 0) {
//       uint32_t hs = mfs->sb.blocksize;
//       if (fill_hole(s, inode, hs, bytes_read)) {
//         return true;
//       }
//       continue;
//     }
// 
//     /* print all of the contents inside that zone*/
//     /* TODO: get all contents in this zone. */
//     if (get_direct_zone_contents(
//           s, 
//           mfs, 
//           inode, 
//           indirect_zone_number, 
//           bytes_read)) {
//       /* If the printing finished somewhere in here, also make note of it by 
//          returning true!*/
//       return true;
//     }
//   }
// 
//   /* We have not yet finished reading. */
//   return false;
// }
// 
// /* Prints the contents of a double indirect zone to a stream s.  
//  * @param s the stream that this message will be printed to.
//  * @param inode the inode of interest. 
//  * @param zone_num the zone of interest.  
//  * @param how many bytes we have read so far.  
//  * @return bool if we have finished writing everything.
//  */
// bool get_two_indirect_zone_contents(
//     FILE* s, 
//     min_fs* mfs, 
//     min_inode* inode, 
//     uint32_t zone_num, 
//     uint32_t* bytes_read) {
//   /* How many zone numbers we are going to read (how many fit in the first 
//      block of the indirect zone) */
//   uint32_t num_indirect_inodes = mfs->sb.blocksize / sizeof(uint32_t);
// 
//   /* We encountered a hole... */
//   if (zone_num == 0) {
//     /* a single indirect node will point to multiple indirect blocks. */
//     /* We must add (blocksize) * how many indirect zone numbers we can fit in 
//        a zone, but then multiply it one more time by that number because there
//        are two layers of indirect nodes we must go through. */
//     /* The size of the hole */
//     uint32_t hs = mfs->sb.blocksize*num_indirect_inodes*num_indirect_inodes;
// 
//     return fill_hole(s, inode, hs, bytes_read);
//   }
// 
//   /* TODO: address*/
//   uint32_t zone_addr = mfs->partition_start + (zone_num*mfs->zone_size);
// 
// 
//   /* For every zone number in that first double indirect inode block. */
//   int i;
//   for(i = 0; i < num_indirect_inodes; i++) {
//     /* The zone number that holds the indierct zone numbers. */
//     uint32_t two_indirect_zone_number;
// 
//     /* Start reading the first block in the double indirect zone. */
//     if (fseek(
//           mfs->file, 
//           zone_addr + (i * sizeof(uint32_t)), 
//           SEEK_SET) == -1) {
//       fprintf(stderr, "error seeking to double indirect zone: %d\n", errno);
//       exit(EXIT_FAILURE);
//     }
// 
//     /* Read the number that holds the zone number. */
//     if(fread(&two_indirect_zone_number, sizeof(uint32_t), 1, mfs->file) < 1) {
//       fprintf(stderr, "error reading double indirect zone number: %d\n", errno);
//       exit(EXIT_FAILURE);
//     }
// 
//     /* Check to see if this is a hole. */
//     if (two_indirect_zone_number == 0) {
//       uint32_t hs = mfs->sb.blocksize*num_indirect_inodes;
//       if (fill_hole(s, inode, hs, bytes_read)) {
//         return true;
//       }
//       continue;
//     }
// 
//     /* Print all of the contents inside that indirect zone*/
//     if (get_indirect_zone_contents(
//           s, 
//           mfs, 
//           inode, 
//           two_indirect_zone_number, 
//           bytes_read)) {
//       /* If the printing finished somewhere in here, also make note of it by 
//          returning true!*/
//       return true;
//     }
//   }
// 
//   /* We have not yet finished reading. */
//   return false;
// }




