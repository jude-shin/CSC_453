#include <stdio.h>
#include <time.h>

#include "messages.h"
#include "disk.h"

/* The max length of a label */
#define LBL_LN 13
/* The max length of a value */
#define VLU_LN 11
/* The lenfth of a hexadecimal value (the "0000" in "0x0000")*/
#define HEX_LN 4
/* The amount of padding needed for a hex value */
#define HEX_PAD VLU_LN - HEX_LN

/* ===== */
/* MINLS */
/* ===== */

/* Prints an error that shows the flags that can be used with minls.
 * @param void. 
 * @return void.
 */
void minls_usage(FILE* s) {
  fprintf(
      s,
      "usage: minls [-v] [-p num [-s num]] imagefile [path]\n"
      "Options:\n"
      "-p part    --- select partition for filesystem (default: none)\n"
      "-s sub     --- select subpartition for filesystem (default: none)\n"
      "-v verbose --- increase verbosity level\n"
      );
}


/* ====== */
/* MINGET */
/* ====== */

/* Prints an error that shows the flags that can be used with minget.
 * @param void. 
 * @return void.
 */
void minget_usage(FILE* s) {
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
 * @param s the FILE that describes the place this will be printed.
 * @param pt a pointer to the inode struct
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
}


/* Prints all of the information in a directory entry.
 * @param dir_entry a pointer to the dir entry struct
 * @return void. 
 */
void print_dir_entry(FILE* s, min_dir_entry* dir_entry) {
  fprintf(s, "%-*s %*u\n", LBL_LN, "inode", VLU_LN, dir_entry->inode);
  fprintf(s, "%-*s %*s\n", LBL_LN, "links", DIR_NAME_SIZE, dir_entry->name); 
}
/* ======= */
/* HELPERS */
/* ======= */

/* Pretty Prints an atime, mtime, or ctime nicely to a FILE*.
 * @param s the FILE that describes the place this will be printed.
 * @param raw_time a number that represents a time and date. 
 * @return void.
 */
void print_time(FILE* s, uint32_t raw_time) {
  time_t t = raw_time;
  fprintf(s, " --- %s", ctime(&t));
}
