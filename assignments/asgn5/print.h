#ifndef MESSAGES
#define MESSAGES

#include "disk.h"

/* For printing a bunch of messages. */

/* ===== */
/* MINLS */
/* ===== */

/* Prints an error that shows the flags that can be used with minls. */
void print_minls_usage(FILE* s);

/* Prints some information about an inode including the rwx permissions for the
   Group, User, and Other, it's size, and it's name. */
void print_file(FILE* s, min_inode* inode, unsigned char* name);

/* Prints every directory entry in a directory. */
void print_directory(FILE* s, min_fs* mfs, min_inode* inode, char* can_path);


/* ====== */
/* MINGET */
/* ====== */

/* Prints an error that shows the flags that can be used with minls. */
void print_minget_usage(FILE* s);


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
