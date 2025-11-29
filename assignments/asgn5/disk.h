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

/* Superblock structure found in fs/super.h in minix 3.1.1. */
typedef struct __attribute__ ((__packed__)) superblock {
  uint32_t ninodes;       /* number of inodes in this filesystem */
  uint16_t pad1;          /* make things line up properly */
  int16_t i_blocks;       /* # of blocks used by inode bit map */
  int16_t z_blocks;       /* # of blocks used by zone bit map */
  uint16_t firstdata;     /* number of first data zone */
  int16_t log_zone_size;  /* log2 of blocks per zone */
  int16_t pad2;           /* make things line up again */
  uint32_t max_file;      /* maximum file size */
  uint32_t zones;         /* number of zones on disk */
  int16_t magic;          /* magic number */
  int16_t pad3;           /* make things line up again */
  uint16_t blocksize;     /* block size in bytes */
  uint8_t subversion;     /* filesystem sub–version */
} superblock;


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

/* Checks to see if an image has both signatures in relation to the offset (This
   allows for subpartitions to be checked also). If they don't, just make note
   and exit with EXIT_FAILURE. */
void validate_signatures(FILE* image, long offset);

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

/* Fills a superblock based ona minix filesystem (a image and an offset) */ 
void load_superblock(min_fs* mfs, superblock* sb);

#endif
