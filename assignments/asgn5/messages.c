#include <stdio.h>

#include "messages.h"
#include "disk.h"

/* ===== */
/* MINLS */
/* ===== */

/* Prints an error that shows the flags that can be used with minls.
 * @param void. 
 * @return void.
 */
void minls_usage(FILE* stream) {
  fprintf(
      stream,
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
void minget_usage(FILE* stream) {
  fprintf(
      stream,
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
void print_part_table(FILE* stream, min_part_tbl* pt) {
  fprintf(stream, "\n\n=== PARTITION TABLE ===============\n");

  fprintf(stream, "bootind: 0x%04x\n", pt->bootind);
  fprintf(stream, "start_head: 0x%04x\n", pt->start_head);
  fprintf(stream, "start_sec: 0x%04x\n", pt->start_sec);
  fprintf(stream, "start_cyl: 0x%04x\n", pt->start_cyl);
  fprintf(stream, "type: 0x%04x\n", pt->type);
  fprintf(stream, "end_head: 0x%04x\n", pt->end_head);
  fprintf(stream, "end_sec: 0x%04x\n", pt->end_sec);
  fprintf(stream, "end_cyl: 0x%04x\n", pt->end_cyl);
  fprintf(stream, "lFirst: 0x%04x\n", pt->lFirst);
  fprintf(stream, "size: %u\n", pt->size);

  fprintf(stream, "=== (end partition table info) ====\n\n");
}


/* Prints all of the information in a superblock.
 * @param pt a pointer to the superblock struct
 * @return void. 
 */
void print_superblock(FILE* stream, min_superblock* sb) {
  fprintf(stream, "\n\n=== SUPERBLOCK ===============\n");

  fprintf(stream, "ninodes: %u\n", sb->ninodes);
  fprintf(stream, "i_blocks: %d\n", sb->i_blocks);
  fprintf(stream, "z_blocks: %d\n", sb->z_blocks);
  fprintf(stream, "firstdata: %u\n", sb->firstdata);
  fprintf(stream, "log_zone_size: %d\n", sb->log_zone_size);
  fprintf(stream, "max_file: %u\n", sb->max_file);
  fprintf(stream, "zones: %u\n", sb->zones);
  fprintf(stream, "magic: 0x%04x\n", sb->magic);
  fprintf(stream, "blocksize: %u\n", sb->blocksize);
  fprintf(stream, "subversion: %u\n", sb->subversion);

  fprintf(stream, "=== (end superblock info) ====\n\n");
}


/* Prints all of the information in a minix inode.
 * @param stream the FILE that describes the place this will be printed.
 * @param pt a pointer to the inode struct
 * @return void. 
 */
void print_inode(FILE* stream, min_inode* inode) {
  fprintf(stream, "\n\n=== INODE ===============\n");

  fprintf(stream, "mode: 0x%04x\n", inode->mode);
  fprintf(stream, "links: %u\n", inode->links);
  fprintf(stream, "uid: %u\n", inode->uid);
  fprintf(stream, "gid: %u\n", inode->gid);
  fprintf(stream, "size: %u\n", inode->size);
  fprintf(stream, "atime: %d\n", inode->atime);
  fprintf(stream, "mtime: %d\n", inode->mtime);
  fprintf(stream, "ctime: %d\n", inode->ctime);
  fprintf(stream, "zone addr: %p\n", (void*)inode->zone); /* TODO: check formatting */
  fprintf(stream, "indirect: %u\n", inode->indirect);
  fprintf(stream, "two_indirect: %u\n", inode->two_indirect);
  fprintf(stream, "unused: %u\n", inode->unused);

  fprintf(stream, "=== (end inode info) ====\n\n");
}


/* Prints all of the information in a directory entry.
 * @param dir_entry a pointer to the dir entry struct
 * @return void. 
 */
void print_dir_entry(FILE* stream, min_dir_entry* dir_entry) {
  fprintf(stream, "\n\n=== DIRECTORY ===============\n");

  fprintf(stream, "inode: %u\n", dir_entry->inode);
  fprintf(stream, "links: %*s\n", DIR_NAME_SIZE, dir_entry->name);

  fprintf(stream, "=== (end directory info) ====\n\n");
}

/* ======= */
/* HELPERS */
/* ======= */

/* Pretty Prints an atime, mtime, or ctime nicely to a FILE*.
 * @param stream the FILE that describes the place this will be printed.
 * @param raw_time a number that represents a time and date. 
*/
void print_time(FILE* stream, uint32_t raw_time) {
  fprintf(stream, "not implemented yet! %d", raw_time);
}





