#include <stdio.h>

#include "messages.h"
#include "disk.h"

/* MINLS */
/* Prints an error that shows the flags that can be used with minls.
 * @param void. 
 * @return void.
 */
void minls_usage(void) {
  fprintf(
      stderr,
      "usage: minls [-v] [-p num [-s num]] imagefile [path]\n"
      "Options:\n"
      "-p part    --- select partition for filesystem (default: none)\n"
      "-s sub     --- select subpartition for filesystem (default: none)\n"
      "-v verbose --- increase verbosity level\n"
      );
}

/* MINGET */
/* Prints an error that shows the flags that can be used with minget.
 * @param void. 
 * @return void.
 */
void minget_usage(void) {
  fprintf(
      stderr,
      "usage: minget [-v] [-p num [-s num]] imagefile srcpath [dstpath]\n"
      "Options:\n"
      "-p part    --- select partition for filesystem (default: none)\n"
      "-s sub     --- select subpartition for filesystem (default: none)\n"
      "-v verbose --- increase verbosity level\n"
      );
}


/* GENERAL */
/* Prints all of the information in a partition table. 
 * @param pt a pointer to the partition table struct
 * @return void. 
 */
void print_part_table(min_part_tbl* pt) {
  fprintf(stderr, "\n\n=== PARTITION TABLE ===============\n\n");

  fprintf(stderr, "bootind: 0x%04x\n", pt->bootind);
  fprintf(stderr, "start_head: 0x%04x\n", pt->start_head);
  fprintf(stderr, "start_sec: 0x%04x\n", pt->start_sec);
  fprintf(stderr, "start_cyl: 0x%04x\n", pt->start_cyl);
  fprintf(stderr, "type: 0x%04x\n", pt->type);
  fprintf(stderr, "end_head: 0x%04x\n", pt->end_head);
  fprintf(stderr, "end_sec: 0x%04x\n", pt->end_sec);
  fprintf(stderr, "end_cyl: 0x%04x\n", pt->end_cyl);
  fprintf(stderr, "lFirst: 0x%04x\n", pt->lFirst);
  fprintf(stderr, "size: %u\n", pt->size);

  fprintf(stderr, "\n=== (end partition table info) ====\n\n");
}


/* Prints all of the information in a superblock.
 * @param pt a pointer to the superblock struct
 * @return void. 
 */
void print_superblock(min_superblock* sb) {
  fprintf(stderr, "\n\n=== SUPERBLOCK ===============\n\n");

  fprintf(stderr, "ninodes: %u\n", sb->ninodes);
  fprintf(stderr, "i_blocks: %d\n", sb->i_blocks);
  fprintf(stderr, "z_blocks: %d\n", sb->z_blocks);
  fprintf(stderr, "firstdata: %u\n", sb->firstdata);
  fprintf(stderr, "log_zone_size: %d\n", sb->log_zone_size);
  fprintf(stderr, "max_file: %u\n", sb->max_file);
  fprintf(stderr, "zones: %u\n", sb->zones);
  fprintf(stderr, "magic: 0x%04x\n", sb->magic);
  fprintf(stderr, "blocksize: %u\n", sb->blocksize);
  fprintf(stderr, "subversion: %u\n", sb->subversion);

  fprintf(stderr, "\n=== (end superblock info) ====\n\n");
}


/* Prints all of the information in a minix inode.
 * @param pt a pointer to the inode struct
 * @return void. 
 */
void print_superblock(min_inode* inode) {
  fprintf(stderr, "\n\n=== SUPERBLOCK ===============\n\n");

  fprintf(stderr, "\n=== (end superblock info) ====\n\n");
}

