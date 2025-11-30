#ifndef MESSAGES
#define MESSAGES

#include "disk.h"

/* For printing a bunch of messages. */

/* ===== */
/* MINLS */
/* ===== */

/* Prints an error that shows the flags that can be used with minls. */
void minls_usage(FILE* stream);


/* ====== */
/* MINGET */
/* ====== */

/* Prints an error that shows the flags that can be used with minls. */
void minget_usage(FILE* stream);


/* ======= */
/* GENERAL */
/* ======= */

/* Prints all of the information in a partition table. */
void print_part_table(FILE* stream, min_part_tbl* pt);

/* Prints all of the information in a superblock. */
void print_superblock(FILE* stream, min_superblock* sb);

/* Prints all of the information in a minix inode. */
void print_inode(FILE* stream, min_inode* inode);

/* Prints all of the information in a directory entry. */
void print_dir_entry(FILE* stream, min_dir_entry* dir_entry);


/* ======= */
/* HELPERS */
/* ======= */

/* Pretty Prints an atime, mtime, or ctime nicely to a FILE*. */
void print_time(FILE* stream, uint32_t raw_time);

#endif
