#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include "print.h"
#include "disk.h"

/* The max length of a label */
#define LBL_LN 13
/* The max length of a value */
#define VLU_LN 11
/* The lenfth of a hexadecimal value (the "0000" in "0x0000")*/
#define HEX_LN 4
/* The amount of padding needed for a hex value */
#define HEX_PAD VLU_LN - HEX_LN

/* The specs when ls'ing a file's information */
#define FILE_SIZE_LN 10

/* The spacing spec for the weird space before the zone. */
#define ZONE_LBL_LEN 9

/* ===== */
/* MINLS */
/* ===== */

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
void print_file(FILE* s, min_inode* inode, unsigned char* name) {
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
void print_files_in_block(
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

      print_file(s, &next_inode, entry.name);
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
void print_files_in_direct_zone(FILE* s, min_fs* mfs, uint32_t zone_num) {
  /* Skip over the indirect zone if it is not used. */
  if (zone_num == 0) { 
    return ;
  }

  /* Read all directory entries in this chunk. */
  uint32_t num_blocks = mfs->zone_size / mfs->sb.blocksize;

  int i;
  for(i = 0; i < num_blocks; i++) {
    /* print ALL blocks that fit in this zone. */
    print_files_in_block(s, mfs, zone_num, i);
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
void print_files_in_indirect_zone(FILE* s, min_fs* mfs, uint32_t zone_num) {
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
    print_files_in_direct_zone(s, mfs, indirect_zone_number);
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
void print_files_in_two_indirect_zone(FILE* s, min_fs* mfs, uint32_t zone_num) {
  /* Skip over the indirect zone if it is not used. */
  if (zone_num == 0) {
    return;
  }

  /* How many zone numbers we are going to read (how many fit in the first 
     block of the indirect zone) */
  uint32_t num_indirect_inodes = mfs->sb.blocksize / sizeof(uint32_t);

  /* TODO: address*/
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
    print_files_in_indirect_zone(s, mfs, indirect_zone_number);
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
void print_directory(FILE* s, min_fs* mfs, min_inode* inode, char* can_path) {
  /* print the full canonicalized minix path that we are listing. */
  fprintf(s, "/%s:\n", can_path);

  /* DIRECT ZONES */
  int i;
  for(i = 0; i < DIRECT_ZONES; i++) {
    /* Print all of the valid files that are in that zone. */
    print_files_in_direct_zone(s, mfs, inode->zone[i]);
  }

  /* INDIRECT ZONES */
  print_files_in_indirect_zone(s, mfs, inode->indirect);

  /* DOUBLE INDIRECT ZONES */
  print_files_in_two_indirect_zone(s, mfs, inode->two_indirect);
}


/* ====== */
/* MINGET */
/* ====== */

/* Prints an error that shows the flags that can be used with minget.
 * @param s the stream that this message will be printed to.
 * @return void.
 */
void print_minget_usage(FILE* s) {
  fprintf(
      s,
      "usage: minget [-v] [-p num [-s num]] imagefile srcpath [dstpath]\n"
      "Options:\n"
      "-p part    --- select partition for filesystem (default: none)\n"
      "-s sub     --- select subpartition for filesystem (default: none)\n"
      "-v verbose --- increase verbosity level\n"
      );
}

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
    if (get_direct_zone_contents(s, mfs, inode, zone_num, &bytes_read)) {
      return;
    }
  }

  /* Go through all of the indirect zones sequentially. */
  if (get_indirect_zone_contents(
        s, 
        mfs, 
        inode, 
        inode->indirect, 
        &bytes_read)) {
    return;
  }

  /* Go through all of the double indirect zones sequentially. */
  if (get_two_indirect_zone_contents(
        s, 
        mfs, 
        inode, 
        inode->two_indirect, 
        &bytes_read)) {
    return;
  }
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

/* Prints the contents of a zone to a stream s.  
 * @param s the stream that this message will be printed to.
 * @param inode the inode of interest. 
 * @param zone_num the zone of interest.  
 * @param how many bytes we have read so far.  
 * @return bool if we have finished writing everything.
 */
bool get_direct_zone_contents(
    FILE* s, 
    min_fs* mfs, 
    min_inode* inode, 
    uint32_t zone_num, 
    uint32_t* bytes_read) {

  /* We encountered a hole... */
  if (zone_num == 0) {
    /* Keep the hole still takes up space, just don't read any of it as it will
       just be zeros. */
    uint32_t hs = mfs->zone_size;

    return fill_hole(s, inode, hs, bytes_read);
  }

  /* get all of the blocks in this zone. */
  uint32_t num_blocks = mfs->zone_size / mfs->sb.blocksize;

  int i;
  for (i = 0; i < num_blocks; i++) {
    if (get_block_contents(s, mfs, inode, zone_num, i, bytes_read)) {
      return true;
    }
  }

  /* We have not yet finished reading. */
  return false;
}

/* Prints the contents of an indirect zone to a stream s.  
 * @param s the stream that this message will be printed to.
 * @param inode the inode of interest. 
 * @param zone_num the zone of interest.  
 * @param how many bytes we have read so far.  
 * @return bool if we have finished writing everything.
 */
bool get_indirect_zone_contents(
    FILE* s, 
    min_fs* mfs, 
    min_inode* inode, 
    uint32_t zone_num, 
    uint32_t* bytes_read) {

  /* How many zone numbers we are going to read (how many fit in the first 
     block of the indirect zone) */
  uint32_t num_indirect_inodes = mfs->sb.blocksize / sizeof(uint32_t);

  /* We encountered a hole... */
  if (zone_num == 0) {
    uint32_t hs = mfs->sb.blocksize*num_indirect_inodes;

    return fill_hole(s, inode, hs, bytes_read);
  }

  /* Start reading the first block in that indirect zone. */
  if (fseek(
        mfs->file, 
        mfs->partition_start+(zone_num*mfs->zone_size), 
        SEEK_SET) == -1) {
    fprintf(stderr, "error seeking to indirect zone.\n");
    exit(EXIT_FAILURE);
  }

  /* The first block is going to line up with the address of that zone. */
  uint32_t block_addr = mfs->partition_start + (zone_num*mfs->zone_size);

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

    if (fread(&indirect_zone_number, sizeof(uint32_t), 1, mfs->file) < 1) {
      fprintf(stderr, "error reading indirect zone: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Check to see if this is a hole. */
    if (indirect_zone_number == 0) {
      uint32_t hs = mfs->sb.blocksize;
      if (fill_hole(s, inode, hs, bytes_read)) {
        return true;
      }
      continue;
    }

    /* print all of the contents inside that zone*/
    /* TODO: get all contents in this zone. */
    if (get_direct_zone_contents(
          s, 
          mfs, 
          inode, 
          indirect_zone_number, 
          bytes_read)) {
      /* If the printing finished somewhere in here, also make note of it by 
         returning true!*/
      return true;
    }
  }

  /* We have not yet finished reading. */
  return false;
}

/* Prints the contents of a double indirect zone to a stream s.  
 * @param s the stream that this message will be printed to.
 * @param inode the inode of interest. 
 * @param zone_num the zone of interest.  
 * @param how many bytes we have read so far.  
 * @return bool if we have finished writing everything.
 */
bool get_two_indirect_zone_contents(
    FILE* s, 
    min_fs* mfs, 
    min_inode* inode, 
    uint32_t zone_num, 
    uint32_t* bytes_read) {
  /* How many zone numbers we are going to read (how many fit in the first 
     block of the indirect zone) */
  uint32_t num_indirect_inodes = mfs->sb.blocksize / sizeof(uint32_t);

  /* We encountered a hole... */
  if (zone_num == 0) {
    /* a single indirect node will point to multiple indirect blocks. */
    /* We must add (blocksize) * how many indirect zone numbers we can fit in 
       a zone, but then multiply it one more time by that number because there
       are two layers of indirect nodes we must go through. */
    /* The size of the hole */
    uint32_t hs = mfs->sb.blocksize*num_indirect_inodes*num_indirect_inodes;

    return fill_hole(s, inode, hs, bytes_read);
  }

  /* TODO: address*/
  uint32_t zone_addr = mfs->partition_start + (zone_num*mfs->zone_size);


  /* For every zone number in that first double indirect inode block. */
  int i;
  for(i = 0; i < num_indirect_inodes; i++) {
    /* The zone number that holds the indierct zone numbers. */
    uint32_t two_indirect_zone_number;

    /* Start reading the first block in the double indirect zone. */
    if (fseek(
          mfs->file, 
          zone_addr + (i * sizeof(uint32_t)), 
          SEEK_SET) == -1) {
      fprintf(stderr, "error seeking to double indirect zone: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Read the number that holds the zone number. */
    if(fread(&two_indirect_zone_number, sizeof(uint32_t), 1, mfs->file) < 1) {
      fprintf(stderr, "error reading double indirect zone number: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Check to see if this is a hole. */
    if (two_indirect_zone_number == 0) {
      uint32_t hs = mfs->sb.blocksize*num_indirect_inodes;
      if (fill_hole(s, inode, hs, bytes_read)) {
        return true;
      }
      continue;
    }

    /* Print all of the contents inside that indirect zone*/
    if (get_indirect_zone_contents(
          s, 
          mfs, 
          inode, 
          two_indirect_zone_number, 
          bytes_read)) {
      /* If the printing finished somewhere in here, also make note of it by 
         returning true!*/
      return true;
    }
  }

  /* We have not yet finished reading. */
  return false;
}


/* ======= */
/* GENERAL */
/* ======= */

/* Prints all of the information in a partition table. 
 * @param s the stream that this message will be printed to.
 * @param pt a pointer to the partition table struct
 * @return void. 
 */
void print_part_table(FILE* s, min_part_tbl* pt) {
  fprintf(s,"Partition Table Contents:\n");
  fprintf(s,"  %-*s %*s%04x\n",LBL_LN,"bootind",HEX_PAD,"0x",pt->bootind);
  fprintf(s,"  %-*s %*s%04x\n",LBL_LN,"start_head",HEX_PAD,"0x",pt->start_head);
  fprintf(s,"  %-*s %*s%04x\n",LBL_LN,"start_sec",HEX_PAD,"0x",pt->start_sec);
  fprintf(s,"  %-*s %*s%04x\n",LBL_LN,"start_cyl",HEX_PAD,"0x",pt->start_cyl);
  fprintf(s,"  %-*s %*s%04x\n",LBL_LN,"type",HEX_PAD,"0x",pt->type);
  fprintf(s,"  %-*s %*s%04x\n",LBL_LN,"end_head",HEX_PAD,"0x",pt->end_head);
  fprintf(s,"  %-*s %*s%04x\n",LBL_LN,"end_sec",HEX_PAD,"0x",pt->end_sec);
  fprintf(s,"  %-*s %*s%04x\n",LBL_LN,"end_cyl",HEX_PAD,"0x",pt->end_cyl);
  fprintf(s,"  %-*s %*s%04x\n",LBL_LN,"lFirst",HEX_PAD,"0x",pt->lFirst);
  fprintf(s,"  %-*s %*u\n",LBL_LN,"size",VLU_LN,pt->size);
  fprintf(s, "\n");
}

/* Prints all of the information in a superblock.
 * @param s the stream that this message will be printed to.
 * @param pt a pointer to the superblock struct
 * @return void. 
 */
void print_superblock(FILE* s, min_superblock* sb) {
  fprintf(s,"Superblock Contents:\n");
  fprintf(s,"Stored Fields:\n");
  fprintf(s,"  %-*s %*u\n", LBL_LN, "ninodes", VLU_LN, sb->ninodes);
  fprintf(s,"  %-*s %*d\n", LBL_LN, "i_blocks", VLU_LN, sb->i_blocks);
  fprintf(s,"  %-*s %*d\n", LBL_LN, "z_blocks", VLU_LN, sb->z_blocks);
  fprintf(s,"  %-*s %*u\n", LBL_LN, "firstdata", VLU_LN, sb->firstdata);
  fprintf(s,"  %-*s %*d\n", LBL_LN, "log_zone_size", VLU_LN, sb->log_zone_size);
  fprintf(s,"  %-*s %*u\n", LBL_LN, "max_file", VLU_LN, sb->max_file);
  fprintf(s,"  %-*s %*u\n", LBL_LN, "zones", VLU_LN, sb->zones);
  fprintf(s,"  %-*s %*s%04x\n", LBL_LN, "magic", HEX_PAD, "0x", sb->magic);
  fprintf(s,"  %-*s %*u\n", LBL_LN, "blocksize", VLU_LN, sb->blocksize);
  fprintf(s,"  %-*s %*u\n", LBL_LN, "subversion", VLU_LN, sb->subversion);
  fprintf(s, "\n");
}

/* Prints all of the information in a minix inode.
 * @param s the stream that this message will be printed to.
 * @param pt a pointer to the inode struct
 * @param inode the inode of interest. 
 * @return void. 
 */
void print_inode(FILE* s, min_inode* inode) {
  fprintf(s, "File inode:\n");
  fprintf(s,"  %-*s %*s%04x\n", LBL_LN, "mode", HEX_PAD, "0x", inode->mode);
  fprintf(s, "  %-*s %*u\n", LBL_LN, "links", VLU_LN, inode->links);
  fprintf(s, "  %-*s %*u\n", LBL_LN, "uid", VLU_LN, inode->uid);
  fprintf(s, "  %-*s %*u\n", LBL_LN, "gid", VLU_LN, inode->gid);
  fprintf(s, "  %-*s %*u\n", LBL_LN, "size", VLU_LN, inode->size);

  fprintf(s, "  %-*s %*d", LBL_LN, "atime", VLU_LN, inode->atime);
  print_time(s, inode->atime);

  fprintf(s, "  %-*s %*d", LBL_LN, "mtime", VLU_LN, inode->mtime);
  print_time(s, inode->mtime);

  fprintf(s, "  %-*s %*d", LBL_LN, "ctime", VLU_LN, inode->ctime);
  print_time(s, inode->ctime);
  fprintf(s, "\n");


  fprintf(s, "  Direct Zones:\n");
  int i;
  for (i = 0; i < DIRECT_ZONES; i++) {
    fprintf(
        s, "  %-*szone[%d]  =%*d\n", 
        ZONE_LBL_LEN, 
        "", i, 
        VLU_LN, 
        inode->zone[i]);
  }

  fprintf(
      s, "  %-*sindirect  %*u\n", 
      ZONE_LBL_LEN, 
      "uint32_t",
      VLU_LN,
      inode->indirect);
  fprintf(
      s, "  %-*sdouble    %*u\n", 
      ZONE_LBL_LEN, 
      "uint32_t", 
      VLU_LN, 
      inode->two_indirect);

  fprintf(s, "\n");
}

/* Prints all of the information in a directory entry.
 * @param s the stream that this message will be printed to.
 * @param dir_entry a pointer to the dir entry struct
 * @return void. 
 */
void print_dir_entry(FILE* s, min_dir_entry* dir_entry) {
  fprintf(s, "Directory Entry:\n");
  fprintf(s, "  %-*s %*u\n", LBL_LN, "inode", VLU_LN, dir_entry->inode);
  fprintf(s, "  %-*s %*s\n", LBL_LN, "links", VLU_LN, dir_entry->name); 
  fprintf(s, "\n");
}


/* ======= */
/* HELPERS */
/* ======= */

/* Pretty Prints an atime, mtime, or ctime nicely to a FILE*.
 * @param s the stream that this message will be printed to.
 * @param raw_time a number that represents a time and date. 
 * @return void.
 */
void print_time(FILE* s, uint32_t raw_time) {
  time_t t = raw_time;
  fprintf(s, " --- %s", ctime(&t));
}

/* Prints c upon a successful bitmask against mode and mask to a stream s.
 * @param s
 * @param c the string that will be printed if we have a successful mask
 * @param mode (the inode->mode) that we will be checking
 * @param mask the macro we will be masking the mode with.
 * @return void.
 */
void print_mask(FILE* s, const char* c, uint16_t mode, uint16_t mask) {
  if (mode & mask) {
    fprintf(s, "%s", c);
  }
  else {
    fprintf(s, "-");
  }
}
