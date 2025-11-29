#ifndef DISK
#define DISK

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* TODO: change "disk.*" to "filesystem.*"*/


/* TODO: everything here will be shared between minls and minget
   This includes the following:
   - check for the valid partition tables (will only be called if a partition 
   is requested)
   - possibly check for valid subpartion tables 
   - check for a valid minix superblock

   - Little endian conversions

   - Check that a file is a directory (done with bitmask)
   - Check that a file is a regular file (done with bitmask)

   - Calculate the size of a zone (done with a bitshift)

   - Canonicalize a path (directoy: bool)


*/
/* ========================================================================== */

/*==================*/
/* DATA DEFINITIONS */
/*==================*/

/* Essentially defines the beginning of the filesystem of an image. 
   It includes the open imagefile filedescriptor, and the offset of where the 
   actual files start. */
typedef struct MinixFileSystem {
  FILE* file;
  size_t partition_start;
  size_t sector_size;
  size_t block_size;
  size_t zone_size;
} min_fs;

/* The struct used as the partition table in a minix MBR filesystem. */
typedef struct __attribute__ ((__packed__)) partition_table {
  uint8_t bootind;    /* Boot magic number (0x80 if bootable). */
  uint8_t start_head; /* Start of partition in CHS. */
  uint8_t start_sec;
  uint8_t start_cyl;
  uint8_t type;       /* Type of partition (0x81 is minix). */
  uint8_t end_head;   /* End of partition in CHS. */
  uint8_t end_sec;
  uint8_t end_cyl;
  uint32_t lFirst;     /* First sector (LBA addressing). */
  uint32_t size;      /* Size of partition (in sectors). */
} part_tbl;


/*==========*/
/* BASIC IO */
/*==========*/

/* Opens the minix filesystem for reading from the imagefile path. This also 
   populates the mfs struct (in this case, the file descriptor) that will be 
   passed around. */
void open_mfs(min_fs* mfs, char* imagefile_path, int prim_part, int sub_part);

/* Closes the minix filesystem. This closes the file descriptor, along with
   cleaning up anything else that is needed. */
void close_mfs(min_fs* mfs);


/*============*/
/* VALIDATION */
/*============*/

/* Checks to see if an image has both signatures. If they don't, just make note
   and exit with EXIT_FAILURE. */
void validate_signatures(FILE* image);

/* Check to see if the partition table holds useful information for this
   assignment. This includes whether an image is bootable, and if the partition
   is from minix. This function does not return anything.
   If the program does not exit after calling this function, then the partition
   table is valid. Otherwise, it will just exit and do nothing. */
void validate_part_table(part_tbl* partition_table);


/*==============*/
/* MISC HELPERS */
/*==============*/

/* Based on an address (primary partition will start at 0, and any subpartition
   will start somewhere else) this function populates the given partition_table
   struct with the data read in the image. Whether the populated data is valid 
   is entirely up to the address variable. */
void load_part_table(part_tbl* partition_table, long addr, FILE* image);

#endif
