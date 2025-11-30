#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include "disk.h"
#include "print.h"

/* TODO: in the helpers, don't pass in the entire context, just pass in the 
   variables that are needed. */

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


/*==========*/
/* BASIC IO */
/*==========*/

/* Opens the imagefile as readonly, and calculates the offset of the specified
 * partition. This populates the filesystem struct and superblock structs that
 * are allocated in the caller.
 * If anything goes wrong, then this function will exit with EXIT_FAILURE. 
 * @param mfs a struct that holds an open file descriptor, and the offset (the
 *  offset from the beginning of the image which indicates the beginning of the
 *  partition that holds the filesystem.
 * @param imagefile_path the host path to the image that we are looking at.
 * @param prim_part what partition number we are interested in. -1 means that
 *  this image is unpartitioned.
 * @param sub_part what subpartition number we are interested in. -1 means that
 *  there are no subpartitions. 
 * @param verbose if this is true, print the partition table's contents
 * @return void. 
 */
void open_mfs(
    min_fs* mfs, 
    char* imagefile_path, 
    int prim_part, 
    int sub_part,
    bool verbose) {
  /* How far from the beginning of the disk the filesystem resides. */
  uint32_t offset;

  /* The (sub)partition table that is read from the image. */
  min_part_tbl pt, spt;
  
  /* The FILE that the image resides in. */
  FILE* imagefile;

  imagefile = fopen(imagefile_path, "rb");
  if (imagefile == 0) {
    fprintf(stderr, "error opening %s: %d\n", imagefile_path, errno);
    exit(EXIT_FAILURE);
  }

  offset = 0;

  /* Seek to the primary partition */
  /* Otherwise treat the image as unpartitioned. (offset of 0) */
  if (prim_part != -1) {

    /* Populate the partition table (pt) */
    load_part_table(&pt, PART_TABLE_OFFSET, imagefile);

    /* Check the signatures, system type, and if this is bootable. */
    validate_part_table(&pt);

    /* Print the contents if you want to. */
    if (verbose) {
      print_part_table(stderr, &pt);
    }

    /* Validate the two signatures in the beginning of the disk. */
    if (!validate_signatures(imagefile, offset)) {
      fprintf(stderr, "primary partition table signatures are not valid\n");
      exit(EXIT_FAILURE);
    }

    /* Calculate where the important part of the partition is. */
    offset = pt.lFirst*SECTOR_SIZE;

    /* Seek to the subpartition */
    if (sub_part != -1) { 
      /* Populate the subpartition table (spt). */
      load_part_table(&spt, offset+PART_TABLE_OFFSET, imagefile);

      /* TODO: ask Ask ASK are there supposed to be signatures here as well? */
      // /* Validate the two signatures in the partition. */
      // if (!validate_signatures(imagefile, offset+PART_TABLE_OFFSET)) {
      //   fprintf(stderr, "subpartition table signatures are not valid\n");
      //   exit(EXIT_FAILURE);
      // }

      /* Check the signatures, system type, and if this is bootable. */
      validate_part_table(&spt);

      /* Print the contents if you want to. */
      if (verbose) {
        fprintf(stderr, "(sub)");
        print_part_table(stderr, &pt);
      }

      /* Calculate where the important part of the partition is. */
      offset = spt.lFirst*SECTOR_SIZE;
    }
  }
  /* if user says it is unpartitioned, but it is really partitioned, it should 
     error. */
  else { /* TODO: ask Ask ASK about this? do I need to check this? */
    /* Check to see if this image had a valid partition table. */
    if (validate_signatures(imagefile, PART_TABLE_OFFSET)) {
      fprintf(stderr, "valid partition table is present! consider using -p\n");
      exit(EXIT_FAILURE);
    }
  }

  /* Update the struct to reflect the file descriptor and offset found. */
  mfs->file = imagefile;
  mfs->partition_start = offset;

  /* At this point we have enough information to update the superblock that is 
     stored in the mfs context! */

  /* Load the superblock. */
  load_superblock(mfs);

  /* TODO: validate the superblock? */

  /* Print the contents if you want to. */
  if (verbose) {
    print_superblock(stderr, &mfs->sb);
  }

  /* Update the mfs context to store the zone size of this filesystem. */
  mfs->zone_size = get_zone_size(&mfs->sb);

  /* Udpate the mfs context to store addresses like the imap, zmap and inodes */

  /* Add the partition start address to the block number * blocksize */
  mfs->b_imap = mfs->partition_start + (IMAP_BLOCK_NUMBER*mfs->sb.blocksize);

  /* Add where the previous block is to the number of blocks in the imap */
  mfs->b_zmap = mfs->b_imap + (mfs->sb.i_blocks*mfs->sb.blocksize);

  /* Add where the previous block is to the number of blocks in the zmap */
  mfs->b_inodes = mfs->b_zmap + (mfs->sb.z_blocks*mfs->sb.blocksize);

  /* The mfs context is now populated with everything we need to know in order
     to traverse the minix filesystem. */
}

/* Closes the file descriptor for the file in the min_fs struct given.
 * @param mfs MinixFileSystem struct that holds the current filesystem. 
 * @return void.
 */
void close_mfs(min_fs* mfs) {
  if (fclose(mfs->file) == -1) {
    fprintf(stderr, "Error close(2)ing the imagefile: %d", errno);
    exit(EXIT_FAILURE);
  }
 }


/*============*/
/* VALIDATION */
/*============*/

/* Check to see if the partition table holds useful information for this
 * assignment. This includes whether an image is bootable, and if the partition
 * is from minix. This function does not return anything.
 * If the program does not exit after calling this function, then the partition
 * table is valid. Otherwise, it will just exit and do nothing.
 * @param partition_table the struct that holds all this information. 
 * @return void.
 */
void validate_part_table(min_part_tbl* partition_table) {
  // /* Check that the image is bootable */
  // if (partition_table->bootind != BOOTABLE_MAGIC) {
  //   fprintf(stderr, "Bad magic number. (%#x)\n", partition_table->bootind);
  //   exit(EXIT_FAILURE);
  // }

  /* Check that the partition type is of minix. */
  if (partition_table->type != MINIX_PARTITION_TYPE) {
    fprintf(stderr, "This is not a minix image.\n");
    exit(EXIT_FAILURE);
  }
}

/* Checks to see if an image has both signatures. If they do return true, else
 * return false.
 * @param image the open FILE that can be read from. 
 * @return bool whether the disk (disk or secondary partition) has the signature
 *  indicating that there is a valid partition table present.
 */
bool validate_signatures(FILE* image, uint32_t offset) {
  unsigned char sig510, sig511;
  
  /* Seek the read head to the address with the signature. */
  fseek(image, offset+SIG510_OFFSET, SEEK_SET);
  /* Read the value at that address. */
  if (fread(&sig510, sizeof(unsigned char), 1, image) < 1) {
    fprintf(stderr, "error reading signature 1: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  /* Seek the read head to the address with the signature. */
  fseek(image, offset+SIG511_OFFSET, SEEK_SET);
  /* Read the value at that address. */
  if (fread(&sig511, sizeof(unsigned char), 1, image) < 1) {
    fprintf(stderr, "error reading signature 2: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  return (sig510 == SIG510_EXPECTED) && (sig511 == SIG511_EXPECTED);
}


/*==============*/
/* MISC HELPERS */
/*==============*/

/* Based on an address (primary partition will start at 0, and any subpartition
 * will start somewhere else) this function populates the given partition table
 * struct with the data read in the image. Whether the populated data is valid 
 * is entirely up to the address variable.
 * @param pt the struct that will be filled (& referenced later).
 * @param addr the address that the partition will start at. 
 * @param image the imagefile that the disk is stored in.
 * @return void.
 */
void load_part_table(min_part_tbl* pt, uint32_t addr, FILE* image) { 
  /* Seek to the correct location that the partition table resides. */
  fseek(image, addr, SEEK_SET);

  /* Read the partition table, storing its contents in the struct for us to 
     reference later on. */
  if (fread(pt, sizeof(min_part_tbl), 1, image) < 1) {
    fprintf(stderr, "error with fread() on partition table: %d\n", errno);
    exit(EXIT_FAILURE);
  }
}

/* Fills a superblock based ona minix filesystem (a image and an offset)
 * @param mfs a struct that holds information; most importantly, the struct for
 *  the superblock.
 * @param verbose if this is true, print the superblock's contents
 * @return void.
 */
void load_superblock(min_fs* mfs) {
  /* Seek to the start of the partition, and then go another SUPERBLOCK_OFFSET
     bytes. No matter the block size, this is where the superblock lives. */
  fseek(mfs->file, mfs->partition_start+SUPERBLOCK_OFFSET, SEEK_SET);

  /* Read the superblock, storing its contents in the struct for us to 
     reference later on. */
  if (fread(&mfs->sb, sizeof(min_superblock), 1, mfs->file) < 1) {
    fprintf(stderr, "error with fread() on superblock: %d\n", errno);
    exit(EXIT_FAILURE);
  }
}

/* ========= */
/* SEARCHING */
/* ========= */

bool search_all_zones(
    min_fs* mfs, 
    min_inode* cur_inode,
    min_inode* next_inode, 
    char* name) {

  /* Search the dierct zones for the current inode (directory) */
  if (search_all_direct_zones(mfs, cur_inode, next_inode, name)) {
    return true;
  }

  /* Search the indierct zones for the current inode (directory) */
  if (search_indirect_zone(mfs, cur_inode->indirect, next_inode, name)) {
    return true;
  }

  /* Search the double indirect zones for the current inode (directory) */
  if (search_two_indirect_zone(
        mfs, 
        cur_inode->two_indirect, 
        next_inode, 
        name)){
    return true;
  }

  return false;
}

/* Searches the direct zones of an indode (which is a directory), and looks for
 * an entry with a corresponding name. If a name is found (and it is not deleted
 * ) populate the next_inode with the contents of the found inode, and return
 * true. Otherwise, return false.
 * @param mfs MinixFileSystem struct that holds the current filesystem and some
 *  useful information.
 * @param cur_inode the inode (which is a directory) that we are searching in.
 * @param next_inode a pointer to the inode that we are going to populate if we
 *  do end up finding the inode.
 * @param name the name of the directory entry we are looking for in the current
 *  inode.
 * @return bool true if we found a valid directory entry, false otherwise.
 */
bool search_all_direct_zones(
    min_fs* mfs, 
    min_inode* cur_inode,
    min_inode* next_inode, 
    char* name) {

  /* Linear search the direct zones. */
  for(int i = 0; i < DIRECT_ZONES; i++) {
    /* Search for the next inode who matches the name of the token. */
    uint32_t zone_num = cur_inode->zone[i];
 
    /* Search the direct zone for an entry, and set the next_inode's values 
       to the inode that was found. Start the search at the start of the
       partition + the offset of zones. */
    bool found = search_zone(mfs, zone_num, next_inode, name);

    /* If found, then we know that search_chunk already wrote the dta to 
       next_inode, so we can just exit. Else, the for loop will continue with
       the search. */
    if (found) {
      return true;
    }
  }

  return false;
}

/* Searches the zones that the indirect zone holds for an entry with a 
 * corresponding name. If a name is found (and it is not deleted) 
 * populate the next_inode with the contents of the found inode, and return
 * true. Otherwise, return false.
 * @param mfs MinixFileSystem struct that holds the current filesystem and some
 *  useful information.
 * @param zone_num the zone number to search.
 * @param next_inode a pointer to the inode that we are going to populate if we
 *  do end up finding the inode.
 * @param name the name of the directory entry we are looking for in the current
 *  inode.
 * @return bool true if we found a valid directory entry, false otherwise.
 */
bool search_indirect_zone(
    min_fs* mfs, 
    uint32_t zone_num,
    min_inode* next_inode, 
    char* name) {

  /* Skip over the indirect zone if it is not used. */
  if (zone_num == 0) {
    return false;
  }

  /* Start reading the first block in that indirect zone. */
  fseek(mfs->file, mfs->partition_start + (zone_num*mfs->zone_size), SEEK_SET);

  /* How many zone numbers we are going to read (how many fit in the first block
     of the indirect zone) */
  int total_indirect_inodes = mfs->sb.blocksize / sizeof(uint32_t);

  /* For every zone number in that first indirect inode block. */
  for(int i = 0; i < total_indirect_inodes; i++) {
    /* The zone number that holds directory entries. */
    uint32_t indirect_zone_number;
   
    /* Read the number that holds the zone number. */
    if(fread(&indirect_zone_number, sizeof(uint32_t), 1, mfs->file) < 1) {
      fprintf(stderr, "error reading indirect zone number: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    // ===============================================================

    /* Search the direct zone for an entry. If found, then we know that 
       search_chunk already wrote the data to next_inode, so we can just exit.
       Otherwise, the for loop will continue with the search. */
    if (search_zone(mfs, indirect_zone_number, next_inode, name)) {
      return true;
    }

    // ===============================================================

    /* Seek to the next indirect zone number to keep searching. */
    fseek(mfs->file, sizeof(uint32_t), SEEK_CUR);
  }

  return false;
}

/* Searches the zones that the double indirect zone holds for an entry with a 
 * corresponding name. If a name is found (and it is not deleted) 
 * populate the next_inode with the contents of the found inode, and return
 * true. Otherwise, return false.
 * @param mfs MinixFileSystem struct that holds the current filesystem and some
 *  useful information.
 * @param zone_num the zone number to search.
 * @param next_inode a pointer to the inode that we are going to populate if we
 *  do end up finding the inode.
 * @param name the name of the directory entry we are looking for in the current
 *  inode.
 * @return bool true if we found a valid directory entry, false otherwise.
 */
bool search_two_indirect_zone(
    min_fs* mfs, 
    uint32_t zone_num,
    min_inode* next_inode, 
    char* name) {

  /* Skip over the indirect zone if it is not used. */
  if (zone_num == 0) {
    return false;
  }

  /* Start reading the first block in the double indirect zone. */
  fseek(mfs->file, mfs->partition_start + (zone_num*mfs->zone_size), SEEK_SET);

  /* How many zone numbers we are going to read (how many fit in the first block
     of the indirect zone) */
  int total_indirect_inodes = mfs->sb.blocksize / sizeof(uint32_t);

  /* For every zone number in that first double indirect inode block. */
  for(int i = 0; i < total_indirect_inodes; i++) {
    /* The zone number that holds the indierct zone numbers. */
    uint32_t two_indirect_zone_number;
   
    /* Read the number that holds the zone number. */
    if(fread(&two_indirect_zone_number, sizeof(uint32_t), 1, mfs->file) < 1) {
      fprintf(stderr, "error reading indirect zone number: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    // ===============================================================

    /* Search the indirect zone for an entry. If found, then we know that 
       search_chunk already wrote the data to next_inode, so we can just exit.
       Otherwise, the for loop will continue with the search. */
    if (search_indirect_zone(mfs, two_indirect_zone_number, next_inode, name)) {
      return true;
    }

    // ===============================================================

    /* Seek to the next two indirect zone number to keep searching. */
    fseek(mfs->file, sizeof(uint32_t), SEEK_CUR);
  }

  return false;
}

/* Searches a zone for a directory entry with a given name. 
 * @param mfs MinixFileSystem struct that holds the current filesystem and some
 *  useful information.
 * @param zone_num the zone number to search.
 * @param next_inode a pointer to the inode that we are going to populate if we
 *  do end up finding the inode.
 * @param name the name of the directory entry we are looking for in the current
 *  inode.
 * @return bool true if we found a valid directory entry, false otherwise.
 */
bool search_zone(
    min_fs* mfs, 
    uint32_t zone_num,
    min_inode* next_inode, 
    char* name) {

  /* Skip over the indirect zone if it is not used. */
  if (zone_num == 0) {
    return false;
  }

  /* The actual address of the zone. */
  uint32_t zone_addr = mfs->partition_start + (zone_num * mfs->zone_size);

  /* Read all directory entries in this chunk. */
  int num_entries = mfs->zone_size / DIR_ENTRY_SIZE;

  for(int j = 0; j < num_entries; j++) {
    min_dir_entry entry;

    /* Seek to the directory entry */
    fseek(mfs->file, zone_addr + (j * DIR_ENTRY_SIZE), SEEK_SET);

    /* Read the directory entry */
    if(fread(&entry, DIR_ENTRY_SIZE, 1, mfs->file) < 1) {
      fprintf(stderr, "error reading directory entry: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Check if entry is valid (inode != 0) and name matches */
    if(entry.inode != 0 && strcmp((char*)entry.name, name) == 0) {
      /* Seek to the address that holds the inode that we are on. */
      fseek(
          mfs->file, 
          mfs->b_inodes + ((entry.inode - 1) * INODE_SIZE),
          SEEK_SET);

      if(fread(next_inode, INODE_SIZE, 1, mfs->file) < 1) {
        /* If there was an error writing to the inode, note it an exit. We 
           could try to limp along, but this is not that critical of an 
           application, so we'll just bail. */
        fprintf(stderr, "error copying the found inode: %d \n", errno);
        exit(EXIT_FAILURE);
      }
  
      return true;
    }
  }

  /* There was no directory entry that had this filename. */
  return false;
}



/* ========== */
/* ARITHMETIC */
/* ========== */

/* Calculates the zonesize based on a minix filesystem context using a bitshift.
 * @param sb a struct that holds information about the superblock.
 * @return uint16_t the size of a zone
 */
uint16_t get_zone_size(min_superblock* sb) {
  uint16_t blocksize = sb->blocksize;
  int16_t log_zone_size = sb->blocksize;
  uint16_t zonesize = blocksize << log_zone_size;

  return zonesize;
}

/* ===== */
/* FILES */
/* ===== */




/* =========== */
/* DIRECTORIES */
/* =========== */

