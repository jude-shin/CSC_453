#ifndef MESSAGES
#define MESSAGES

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


/* ======= */
/* GENERAL */
/* ======= */

/* Prints all of the information in a partition table. */
void print_part_table(FILE* s, min_part_tbl* pt);

/* Prints all of the information in a superblock. */
void print_superblock(FILE* s, min_superblock* sb);

/* Prints all of the information in a minix inode. */
void print_inode(FILE* s, min_inode* inode);

/* Prints all of the information in a directory entry. */
void print_dir_entry(FILE* s, min_dir_entry* dir_entry);


/* ======= */
/* HELPERS */
/* ======= */

/* Pretty Prints an atime, mtime, or ctime nicely to a FILE*. */
void print_time(FILE* s, uint32_t raw_time);

/* Prints c upon a successful bitmask against mode and mask to a stream s. */
void print_mask(FILE* s, const char* c, uint16_t mode, uint16_t mask);

#endif
