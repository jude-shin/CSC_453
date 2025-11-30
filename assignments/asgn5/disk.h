#ifndef DISK
#define DISK

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* ============== */
/* FILETYPE MASKS */
/* ============== */
#define DIR_FT 0040000
#define REG_FT 0100000

/* ===================== */
/* MINIX ARBITRARY SIZES */
/* ===================== */
/* How many zones there are in a minix inode. */
#define DIRECT_ZONES 7
/* The max size of a directoy entry name. */
#define DIR_NAME_SIZE 60

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

/* The struct used as the partition table in a minix MBR filesystem. */
typedef struct __attribute__ ((__packed__)) min_part_tbl {
  uint8_t bootind;    /* Boot magic number (0x80 if bootable). */
  uint8_t start_head; /* Start of partition in CHS. */
  uint8_t start_sec;
  uint8_t start_cyl;
  uint8_t type;       /* Type of partition (0x81 is minix). */
  uint8_t end_head;   /* End of partition in CHS. */
  uint8_t end_sec;
  uint8_t end_cyl;
  uint32_t lFirst;     /* First sector (LBA addressing). */
  uint32_t size;       /* Size of partition (in sectors). */
} min_part_tbl;

/* Superblock structure found in fs/super.h in minix 3.1.1. */
typedef struct __attribute__ ((__packed__)) min_superblock {
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
} min_superblock;

/* An inode in minix. */
typedef struct __attribute__ ((__packed__)) min_inode {
  uint16_t mode;    /* mode */
  uint16_t links;   /* number or links */
  uint16_t uid;
  uint16_t gid;
  uint32_t size;
  int32_t atime;
  int32_t mtime;
  int32_t ctime;
  uint32_t zone[DIRECT_ZONES];
  uint32_t indirect;
  uint32_t two_indirect;
  uint32_t unused;
} min_inode;

/* A directoy entry in minix. */
typedef struct __attribute__ ((__packed__)) min_dir_entry {
  uint32_t inode;                     /* Inode Number */
  unsigned char name[DIR_NAME_SIZE];  /* filename string */
} min_dir_entry;

/* Essentially defines the beginning of the filesystem of an image. 
   It includes the open imagefile filedescriptor, and the offset of where the 
   actual files start. */
typedef struct min_fs {
  FILE* file;             /* the file that the (minix) image is on */
  uint32_t partition_start; /* the offset (the address in relation to the 
                             beginning of the image file. aka "real" address)*/
  min_superblock sb;      /* The superblock and it's information for the fs */

  uint16_t zone_size;     /* The size of a zone in bytes */

  uint32_t b_imap;        /* "real" address of the inode bitmap */
  uint32_t b_zmap;        /* "real" address of the zone bitmap */
  uint32_t b_inodes;      /* "real" address of the actual inodes (first addr)*/
} min_fs;


/*==========*/
/* BASIC IO */
/*==========*/

/* Opens the imagefile as readonly, and calculates the offset of the specified
   partition. This populates the filesystem struct and superblock structs that
   are allocated in the caller.
   If anything goes wrong, then this function will exit with EXIT_FAILURE. */
void open_mfs(
    min_fs* mfs, 
    char* imagefile_path, 
    int prim_part, 
    int sub_part,
    bool verbose);

/* Closes the minix filesystem. This closes the file descriptor, along with
   cleaning up anything else that is needed. */
void close_mfs(min_fs* mfs);


/*============*/
/* VALIDATION */
/*============*/

/* Checks to see if an image has both signatures in relation to the offset (This
   allows for subpartitions to be checked also). If they do, return true, else
   return false. */
bool validate_signatures(FILE* image, long offset);

/* Check to see if the partition table holds useful information for this
   assignment. This includes whether an image is bootable, and if the partition
   is from minix. This function does not return anything.
   If the program does not exit after calling this function, then the partition
   table is valid. Otherwise, it will just exit and do nothing. */
void validate_part_table(min_part_tbl* partition_table);


/*==============*/
/* MISC HELPERS */
/*==============*/

/* Based on an address (primary partition will start at 0, and any subpartition
   will start somewhere else) this function populates the given partition_table
   struct with the data read in the image. Whether the populated data is valid 
   is entirely up to the address variable. */
void load_part_table(min_part_tbl* pt, long addr, FILE* image, bool verbose);

/* Fills a superblock based ona minix filesystem (a image and an offset) */ 
void load_superblock(min_fs* mfs, bool verbose);

/* TODO: comments */
bool search_direct_zones(
    min_fs* mfs, 
    min_inode* cur_inode,
    min_inode* next_inode, 
    char* name);

/* TODO: comments */
bool search_chunk_for_dir_entry(
    min_fs* mfs, 
    uint32_t start_addr,
    uint32_t chunk_size,
    min_inode* next_inode, 
    char* name);


/* ========== */
/* ARITHMETIC */
/* ========== */

/* Calculates the zonesize based on a superblock using a bitshift. */
uint16_t get_zone_size(min_superblock* sb);


/* ===== */
/* FILES */
/* ===== */



/* =========== */
/* DIRECTORIES */
/* =========== */


#endif
