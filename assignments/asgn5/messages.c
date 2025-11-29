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
void print_part_table(part_tbl* pt) {
  printf("\n\n=== PARTITION TABLE ===============\n\n");

  printf("bootind: 0x%04x\n", pt->bootind);
  printf("start_head: 0x%04x\n", pt->start_head);
  printf("start_sec: 0x%04x\n", pt->start_sec);
  printf("start_cyl: 0x%04x\n", pt->start_cyl);
  printf("type: 0x%04x\n", pt->type);
  printf("end_head: 0x%04x\n", pt->end_head);
  printf("end_sec: 0x%04x\n", pt->end_sec);
  printf("end_cyl: 0x%04x\n", pt->end_cyl);
  printf("lFirst: 0x%04x\n", pt->lFirst);
  printf("size: %u\n", pt->size);

  printf("\n=== (end partition table info) ====\n\n");
}


/* Prints all of the information in a superblock.
 * @param pt a pointer to the superblock struct
 * @return void. 
 */
void print_superblock(superblock* sb) {
  printf("\n\n=== SUPERBLOCK ===============\n\n");

  printf("ninodes: %u\n", sb->ninodes);
  printf("i_blocks: %d\n", sb->i_blocks);
  printf("z_blocks: %d\n", sb->z_blocks);
  printf("firstdata: %u\n", sb->firstdata);
  printf("log_zone_size: %d\n", sb->log_zone_size);
  printf("max_file: %u\n", sb->max_file);
  printf("zones: %u\n", sb->zones);
  printf("magic: 0x%04x\n", sb->magic);
  printf("blocksize: %u\n", sb->blocksize);
  printf("subversion: %u\n", sb->subversion);

  printf("\n=== (end superblock info) ====\n\n");
}

