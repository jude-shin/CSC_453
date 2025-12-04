#ifndef DISK
#define DISK

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* ============== */
/* FILETYPE MASKS */
/* ============== */
/* Directory Mask */
#define DIR_FT 0040000
/* Regular File  Mask */
#define REG_FT 0100000
/* Filetype Mask */
#define FILE_TYPE 0170000


/* ================ */
/* PERMISSION MASKS */
/* ================ */
#define OWNER_R_PEM 0000400
#define OWNER_W_PEM 0000200
#define OWNER_X_PEM 0000100

#define GROUP_R_PEM 0000040
#define GROUP_W_PEM 0000020
#define GROUP_X_PEM 0000010

#define OTHER_R_PEM 0000004
#define OTHER_W_PEM 0000002
#define OTHER_X_PEM 0000001


/* ===================== */
/* MINIX ARBITRARY SIZES */
/* ===================== */
/* How many zones there are in a minix inode. */
#define DIRECT_ZONES 7

/* The max size of a directoy entry name. */
#define DIR_NAME_SIZE 60

/* The inode size for a minix fs in bytes. */
#define INODE_SIZE 64

/* The directory entry size for a minix fs in bytes. */
#define DIR_ENTRY_SIZE 64


/* ==== */
/* MISC */
/* ==== */
/* The delimiter when listing directories. */
#define DELIMITER "/"


/* ================= */
/* ADDRESS CONSTANTS */
/* ================= */
/* The address where the partition table is. */
#define PART_TABLE_OFFSET 0x1BE 

/* Where the superblock lies in relation to the beginning of the partition. */
#define SUPERBLOCK_OFFSET 1024

/* Addresses for particular signatures in a minix (sub)partition. */
#define SIG510_OFFSET 510 
#define SIG511_OFFSET 511


/* ============== */
/* SIZE CONSTANTS */
/* ============== */
/* The sector size for a minix fs in bytes. */
#define SECTOR_SIZE 512 

/* The block number for the imap block in a minix fs. */
#define IMAP_BLOCK_NUMBER 2


/* ================ */
/* MAGIC VALIDATION */
/* ================ */
/* Bootind will be equal to this macro if the partition is bootable */
#define BOOTABLE_MAGIC 0x80

/* The partition type that indicates this is a minix partition */
#define MINIX_PARTITION_TYPE 0x81

/* Magic value will be in the superblock, indicating that this is a minix fs. */
#define MINIX_MAGIC_NUMBER 0x4D5A

/* The expected values for particular signatures in a minix (sub)partition. */
#define SIG510_EXPECTED 0x55 
#define SIG511_EXPECTED 0xAA


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
  uint32_t lFirst;    /* First sector (LBA addressing). */
  uint32_t size;      /* Size of partition (in sectors). */
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

  uint32_t zone_size;     /* The size of a zone in bytes */
  /* TODO: add here some calculations like number of addresses in ablock or 
     something*/

  uint32_t b_imap;        /* "real" address of the inode bitmap */
  uint32_t b_zmap;        /* "real" address of the zone bitmap */
  uint32_t b_inodes;      /* "real" address of the actual inodes (first addr)*/
} min_fs;


/*==========*/
/* BASIC IO */
/*==========*/
/* Opens the imagefile as readonly, and sets up datastructures like the mfs
   context. If anything goes wrong, then this function will exit with 
   EXIT_FAILURE. */
void open_mfs(
    min_fs* mfs, 
    char* imagefile_path, 
    int p_prt, 
    int s_prt,
    bool verb);

/* Closes the minix filesystem. This closes the file descriptor, along with
   cleaning up anything else that is needed. */
void close_mfs(min_fs* mfs);


/*============*/
/* VALIDATION */
/*============*/
/* Checks to see if an image has both signatures in relation to the offset (This
   allows for subpartitions to be checked also). If they do, return true, else
   return false. */
bool validate_signatures(FILE* image, uint32_t offset);


/*==============*/
/* MISC HELPERS */
/*==============*/
/* Based on an address (primary partition will start at 0, and any subpartition
   will start somewhere else) this function populates the given partition_table
   struct with the data read in the image. Whether the populated data is valid 
   is entirely up to the address variable. */
void load_part_table(min_part_tbl* pt, uint32_t addr, FILE* image);

/* Fills a superblock based ona minix filesystem (a image and an offset) */ 
void load_superblock(min_fs* mfs);

/* Makes a copy of an inode based on an arbitrary address.*/
void duplicate_inode(min_fs* mfs, uint32_t inode_addr, min_inode* inode);


/* ========= */
/* SEARCHING */
/* ========= */
/* Populates inode_addr with the given inode's address and returns true if it 
   was found. Otherwise, return false. The canonicalized path that was traversed
   is also built as this function progresses, as well as the cur_name being 
   updated. */
uint32_t find_inode(
    min_fs* mfs, 
    min_inode* inode,
    char* path,
    char* can_minix_path,
    unsigned char* cur_name);


/* Searches a block to find a directory entry with a mathcing name. 
   Conforms to the block_processor function and can be used as a callback. */
bool search_block(
    FILE* s, /* We dont need to have any stream for the output. */
    min_fs* mfs, 
    min_inode* inode, 
    uint32_t zone_num,
    uint32_t block_number,
    void* name);

/* Searches the direct, indirect, and double indirect zones of the cur_inode 
   (which is a directory), and looks for an entry with a corresponding name. 
   If a name is found (and it is not deleted) populate inode with the
   contents of the found inode, and return true, otherwise, return false. */
bool search_all_zones(
    min_fs* mfs, 
    min_inode* inode, 
    min_inode* cur_inode,
    char* name);


/* ========== */
/* ARITHMETIC */
/* ========== */
/* Calculates the zonesize based on a superblock using a bitshift. */
uint32_t get_zone_size(min_superblock* sb);

/* Calculates the zone address on a minix filesystem. */
uint32_t get_zone_addr(min_fs* mfs, uint32_t zone_num);

/* Calculates the block address on a minix filesystem. */
uint32_t get_block_addr(min_fs* mfs, uint32_t zone_num, uint32_t block_num);


#endif
