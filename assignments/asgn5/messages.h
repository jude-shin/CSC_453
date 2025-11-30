#ifndef MESSAGES
#define MESSAGES

#include "disk.h"

/* For printing a bunch of messages. */

/* MINLS */
/* Prints an error that shows the flags that can be used with minls. */
void minls_usage(void);

/* MINGET */
/* Prints an error that shows the flags that can be used with minls. */
void minget_usage(void);

/* GENERAL */
/* Prints all of the information in a partition table. */
void print_part_table(min_part_tbl* pt);

/* Prints all of the information in a superblock. */
void print_superblock(min_superblock* sb);

/* Prints all of the information in a minix inode. */
void print_inode(min_inode* inode);

/* Prints all of the information in a directory entry. */
void print_inode(min_dir* dir);

#endif
