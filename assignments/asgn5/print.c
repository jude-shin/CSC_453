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
#define FILE_SIZE_LN 9

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

/* Given a zone, print all of the files that are on that zone if they are valid.
 * @param s the stream that this message will be printed to.
 * @param mfs a struct that holds an open file descriptor, and the offset (the
 *  offset from the beginning of the image which indicates the beginning of the
 *  partition that holds the filesystem.
 * @param zone_num the zone number of interest
 * @return void.
 */
void print_files_in_zone(FILE* s, min_fs* mfs, uint32_t zone_num) {
  /* Skip over the indirect zone if it is not used. */
  if (zone_num != 0) {

    /* The actual address of the zone. */
    uint32_t zone_addr = mfs->partition_start + (zone_num * mfs->zone_size);

    /* Read all directory entries in this chunk. */
    int num_entries = mfs->zone_size / DIR_ENTRY_SIZE;

    for(int j = 0; j < num_entries; j++) {
      min_dir_entry entry;

      /* Seek to the directory entry */
      fseek(mfs->file, zone_addr + (j * DIR_ENTRY_SIZE), SEEK_SET);

      /* Read the directory entry */
      if(fread(&entry, DIR_ENTRY_SIZE, 1, mfs->file) < 1) {
        fprintf(stderr, "error reading directory entry: %d\n", errno);
        exit(EXIT_FAILURE);
      }

      /* Check if entry is valid */
      if(entry.inode != 0) {
        min_inode next_inode;

        /* Seek to the address that holds the inode that we are on. */
        fseek(
            mfs->file, 
            mfs->b_inodes + ((entry.inode - 1) * INODE_SIZE),
            SEEK_SET);

        /* Fill the next_inode with the found informatio. */
        if(fread(&next_inode, INODE_SIZE, 1, mfs->file) < 1) {
          /* If there was an error writing to the inode, note it an exit. We 
             could try to limp along, but this is not that critical of an 
             application, so we'll just bail. */
          fprintf(stderr, "error copying the found inode: %d \n", errno);
          exit(EXIT_FAILURE);
        }

        print_file(s, &next_inode, entry.name);
      }
    }
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
  fprintf(s, "%s:\n", can_path);

  /* DIRECT ZONES */
  for(int i = 0; i < DIRECT_ZONES; i++) {
    /* Search for the next inode who matches the name of the token. */
    uint32_t zone_num = inode->zone[i];

    /* Print all of the valid files that are in that zone. */
    print_files_in_zone(s, mfs, zone_num);
  }

  /* INDIRECT ZONES */
  /* Skip over the indirect zone if it is not used. */
  if (inode->indirect != 0) {
    /* Start reading the first block in that indirect zone. */
    fseek(
        mfs->file, 
        mfs->partition_start + (inode->indirect*mfs->zone_size), 
        SEEK_SET);

    /* How many zone numbers we are going to read (how many fit in the first 
       block of the indirect zone) */
    int total_indirect_inodes = mfs->sb.blocksize / sizeof(uint32_t);

    /* For every zone number in that first indirect inode block. */
    for(int i = 0; i < total_indirect_inodes; i++) {
      /* The zone number that holds directory entries. */
      uint32_t indirect_zone_number;

      /* Read the number that holds the zone number. */
      if(fread(&indirect_zone_number, sizeof(uint32_t), 1, mfs->file) < 1) {
        fprintf(stderr, "error reading indirect zone number: %d\n", errno);
        exit(EXIT_FAILURE);
      }

      /* Print all of the valid files that are in the zones pointed to in
         the indirect zone. */
      print_files_in_zone(s, mfs, indirect_zone_number);

      /* Seek to the next indirect zone number to keep searching. */
      fseek(mfs->file, sizeof(uint32_t), SEEK_CUR);
    }
  }

  /* TODO: DOUBLE INDIRECT ZONES */
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
  fprintf(s,"\t%-*s %*s%04x\n",LBL_LN,"bootind",HEX_PAD,"0x",pt->bootind);
  fprintf(s,"\t%-*s %*s%04x\n",LBL_LN,"start_head",HEX_PAD,"0x",pt->start_head);
  fprintf(s,"\t%-*s %*s%04x\n",LBL_LN,"start_sec",HEX_PAD,"0x",pt->start_sec);
  fprintf(s,"\t%-*s %*s%04x\n",LBL_LN,"start_cyl",HEX_PAD,"0x",pt->start_cyl);
  fprintf(s,"\t%-*s %*s%04x\n",LBL_LN,"type",HEX_PAD,"0x",pt->type);
  fprintf(s,"\t%-*s %*s%04x\n",LBL_LN,"end_head",HEX_PAD,"0x",pt->end_head);
  fprintf(s,"\t%-*s %*s%04x\n",LBL_LN,"end_sec",HEX_PAD,"0x",pt->end_sec);
  fprintf(s,"\t%-*s %*s%04x\n",LBL_LN,"end_cyl",HEX_PAD,"0x",pt->end_cyl);
  fprintf(s,"\t%-*s %*s%04x\n",LBL_LN,"lFirst",HEX_PAD,"0x",pt->lFirst);
  fprintf(s,"\t%-*s %*u\n",LBL_LN,"size",VLU_LN,pt->size);
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
  fprintf(s,"\t%-*s %*u\n", LBL_LN, "ninodes", VLU_LN, sb->ninodes);
  fprintf(s,"\t%-*s %*d\n", LBL_LN, "i_blocks", VLU_LN, sb->i_blocks);
  fprintf(s,"\t%-*s %*d\n", LBL_LN, "z_blocks", VLU_LN, sb->z_blocks);
  fprintf(s,"\t%-*s %*u\n", LBL_LN, "firstdata", VLU_LN, sb->firstdata);
  fprintf(s,"\t%-*s %*d\n", LBL_LN, "log_zone_size", VLU_LN, sb->log_zone_size);
  fprintf(s,"\t%-*s %*u\n", LBL_LN, "max_file", VLU_LN, sb->max_file);
  fprintf(s,"\t%-*s %*u\n", LBL_LN, "zones", VLU_LN, sb->zones);
  fprintf(s,"\t%-*s %*s%04x\n", LBL_LN, "magic", HEX_PAD, "0x", sb->magic);
  fprintf(s,"\t%-*s %*u\n", LBL_LN, "blocksize", VLU_LN, sb->blocksize);
  fprintf(s,"\t%-*s %*u\n", LBL_LN, "subversion", VLU_LN, sb->subversion);
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
  fprintf(s,"\t%-*s %*s%04x\n", LBL_LN, "mode", HEX_PAD, "0x", inode->mode);
  fprintf(s, "\t%-*s %*u\n", LBL_LN, "links", VLU_LN, inode->links);
  fprintf(s, "\t%-*s %*u\n", LBL_LN, "uid", VLU_LN, inode->uid);
  fprintf(s, "\t%-*s %*u\n", LBL_LN, "gid", VLU_LN, inode->gid);
  fprintf(s, "\t%-*s %*u\n", LBL_LN, "size", VLU_LN, inode->size);

  fprintf(s, "\t%-*s %*d", LBL_LN, "atime", VLU_LN, inode->atime);
  print_time(s, inode->atime);

  fprintf(s, "\t%-*s %*d", LBL_LN, "mtime", VLU_LN, inode->mtime);
  print_time(s, inode->mtime);

  fprintf(s, "\t%-*s %*d", LBL_LN, "ctime", VLU_LN, inode->ctime);
  print_time(s, inode->ctime);
  fprintf(s, "\n");


  /* TODO: print the zone things*/
}


/* Prints all of the information in a directory entry.
 * @param s the stream that this message will be printed to.
 * @param dir_entry a pointer to the dir entry struct
 * @return void. 
 */
void print_dir_entry(FILE* s, min_dir_entry* dir_entry) {
  fprintf(s, "Directory Entry:\n");
  fprintf(s, "\t%-*s %*u\n", LBL_LN, "inode", VLU_LN, dir_entry->inode);
  fprintf(s, "\t%-*s %*s\n", LBL_LN, "links", VLU_LN, dir_entry->name); 
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
