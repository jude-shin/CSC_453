#include <stdio.h>
#include <time.h>

#include "print.h"
#include "disk.h"

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
