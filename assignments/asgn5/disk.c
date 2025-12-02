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
  uint32_t offset = 0;

  /* The (sub)partition table that is read from the image. */
  min_part_tbl pt, spt;
  
  /* The FILE that the image resides in. */
  FILE* imagefile;

  imagefile = fopen(imagefile_path, "rwb");
  if (imagefile == 0) {
    fprintf(stderr, "error opening %s: %d\n", imagefile_path, errno);
    exit(EXIT_FAILURE);
  }

  /* Seek to the primary partition */
  /* Otherwise treat the image as unpartitioned. (offset of 0) */
  if (prim_part != -1) {
    /* Populate the partition table (pt) based on what partition was chosen. */
    load_part_table(
        &pt, 
        (prim_part*sizeof(min_part_tbl)) + PART_TABLE_OFFSET, 
        imagefile);

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
    /* Populate the subpartition table (spt) based on what subpartition was 
       chosen. */
      load_part_table(
          &spt, 
          offset+(sub_part*sizeof(min_part_tbl))+PART_TABLE_OFFSET, 
          imagefile);

      /* Validate the two signatures in the partition. */
      if (!validate_signatures(imagefile, offset)) {
        fprintf(stderr, "subpartition table signatures are not valid\n");
        exit(EXIT_FAILURE);
      }

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

  /* Update the struct to reflect the file descriptor and offset found. */
  mfs->file = imagefile;
  mfs->partition_start = offset;

  /* At this point we have enough information to update the superblock that is 
     stored in the mfs context! */

  /* Load the superblock. */
  load_superblock(mfs);

  /* Validate the superblock (make sure that the magic minix number is there )*/
  if (mfs->sb.magic != MINIX_MAGIC_NUMBER) {
    fprintf(stderr, "bad superblock (not a minix filesystem)\n");
    exit(EXIT_FAILURE);
  }

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
  /* TODO: put the malloced canonicalized string in here? then we can just
     close everything, cleaning up the malloced thing? */
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
  if (fseek(image, offset+SIG510_OFFSET, SEEK_SET) == -1) {
    fprintf(stderr, "error seeking to signature 1: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  /* Read the value at that address. */
  if (fread(&sig510, sizeof(unsigned char), 1, image) < 1) {
    fprintf(stderr, "error reading signature 1: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  /* Seek the read head to the address with the signature. */
  if (fseek(image, offset+SIG511_OFFSET, SEEK_SET) == -1) {
    fprintf(stderr, "error seeking to signature 2: %d\n", errno);
    exit(EXIT_FAILURE);
  }
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
  if (fseek(image, addr, SEEK_SET)) {
    fprintf(stderr, "error seeking to partition table: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  /* Read the partition table, storing its contents in the struct for us to 
     reference later on. */
  if (fread(pt, sizeof(min_part_tbl), 1, image) < 1) {
    fprintf(stderr, "error reading partition table: %d\n", errno);
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
  if (fseek(mfs->file, mfs->partition_start+SUPERBLOCK_OFFSET, SEEK_SET) == -1){
    fprintf(stderr, "error seeking to superblock: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  /* Read the superblock, storing its contents in the struct for us to 
     reference later on. */
  if (fread(&mfs->sb, sizeof(min_superblock), 1, mfs->file) < 1) {
    fprintf(stderr, "error reading superblock: %d\n", errno);
    exit(EXIT_FAILURE);
  }
}

/* Makes a copy of an inode based on an arbitrary address. 
 * @param mfs
 * @param inode_addr the address of the real inode on the image
 * @param inode a ponter to a struct in my program that will hold the copy
 * @return void.
*/
void duplicate_inode(min_fs* mfs, uint32_t inode_addr, min_inode* inode) {
  /* Seek to the inode's address */ 
  if (fseek(mfs->file, inode_addr, SEEK_SET) == -1) {
    fprintf(stderr, "error seeking to the found inode: %d\n", errno);
    exit(EXIT_FAILURE);
  }
  /* Read the value at that address into the root inode struct. */
  if (fread(inode, sizeof(min_inode), 1, mfs->file) < 1) {
    fprintf(stderr, "error copying found inode: %d\n", errno);
    exit(EXIT_FAILURE);
  }
}

/* Populates inode with the given inode's information and returns true if it was
 * found. Otherwise, return false. The canonicalized path that was traversed is
 * also built as this function progresses, as well as the cur_name being updated
 * .
 * @param mfs MinixFileSystem struct that holds the current filesystem and some
 *  useful information.
 * @param inode the inode that will be filled once the file is found.
 * @param path the name of the path that is to be followed
 * @param can_minix_path the canonicalized minix path that is built as the
 *  the search progresses. Note that if the file is the root, then 
 *  can_minix_path does not change. If this is set to NULL, no canonicalized
 *  path is built. 
 * @param cur_name the current name that is being processed. If this is set to
 * NULL, the name is not updated. 
 * @return the address to the located inode. If not found, return 0;
   */
uint32_t find_inode(
    min_fs* mfs, 
    uint32_t* inode_addr,
    char* path,
    char* can_minix_path,
    unsigned char* cur_name) {

  /* The tokenized next directory entry name that we are looking for. */
  /* TODO: try strtok_r?*/
  char* token = strtok(path, DELIMITER);
 
  /* Set the default address to the root inode's address. */
  *inode_addr = mfs->b_inodes;

  /* The current inode we are processing. */
  min_inode inode;

  /* Seek the read head to the first inode. */
  if (fseek(mfs->file, mfs->b_inodes, SEEK_SET) == -1) {
    fprintf(stderr, "error seeking to the root inode: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  /* Read the value at that address into the root inode struct. */
  if (fread(&inode, sizeof(min_inode), 1, mfs->file) < 1) {
    fprintf(stderr, "error reading the root inode: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  /* Parse all of the directories that the user gave by traversing through the
     directories till we are at the last inode. */
  while(token != NULL) {
    /* Make sure that there isn't a filename with a length that is greater than
       DIR_NAME_SIZE*/
    if (strlen(token) > DIR_NAME_SIZE) {
      exit(EXIT_FAILURE);
    }

    /* copy the string name to cur_name so we can keep track of the last
       processed name. */
    if (cur_name != NULL) {
      size_t len = strlen(token);
      if (len >= DIR_NAME_SIZE) {
        len = DIR_NAME_SIZE - 1;
      }
      memcpy(cur_name, token, len);
      cur_name[len] = '\0';
    }

    /* Add the token to the built canonicalized minix path. */
    if (can_minix_path != NULL) {
      strcat(can_minix_path, token);
      strcat(can_minix_path, DELIMITER);
    }

    /* The current inode must be traversable (a directory) */
    if (!(inode.mode & DIR_FT)) {
      fprintf(
          stderr, 
          "error traversing the path. %s is not a directory!\n", 
          token);
      return false;
    }
   
    /* Search through the direct, indirect, and double indirect zones for a 
       directory entry with a matching name. */
    if (search_all_zones(mfs, &inode, inode_addr, token)) {
      /* Overwrite the inode with the contents in the image at inode_addr. */
      duplicate_inode(mfs, *inode_addr, &inode);
      token = strtok(NULL, DELIMITER);
    }
    else {
      fprintf(stderr, "error traversing the path: directory not found!\n");
      return false;
    }
  }
  return true;
}

/* ========= */
/* SEARCHING */
/* ========= */

/* Searches the direct, indirect, and double indirect zones of an indode 
 * (which is a directory), and looks for an entry with a corresponding name. 
 * If a name is found (and it is not deleted) populate the next_inode with the
 * contents of the found inode, and return true, otherwise, return false. 
 * @param mfs MinixFileSystem struct that holds the current filesystem and some
 *  useful information.
 * @param cur_inode the inode (which is a directory) that we are searching in.
 * @param inode_addr a poninter to a variable that holds the address of the real
 *  inode on the image. 
 * @param name the name of the directory entry we are looking for in the current
 *  inode.
 * @return bool true if we found a valid directory entry, false otherwise.
 */
bool search_all_zones(
    min_fs* mfs, 
    min_inode* cur_inode,
    uint32_t* inode_addr, 
    char* name) {
  int i;

  /* Linear search the direct zones. */
  for(i = 0; i < DIRECT_ZONES; i++) {
    /* Search the direct zone for an entry. If found, then we know that 
       search_chunk already wrote the data to inode_addr, so we can just exit.
       Otherwise, the for loop will continue with the search. */
    if (search_zone(mfs, cur_inode->zone[i], inode_addr, name)) {
      return true;
    }
  }

  /* Search the indirect zone for an entry. If found, then we know that 
     search_chunk already wrote the data to inode_addr, so we can just exit.
     Otherwise, the for loop will continue with the search. */
  if (search_indirect_zone(mfs, cur_inode->indirect, inode_addr, name)) {
    return true;
  }

  /* Search the double indirect zone for an entry. If found, then we know that 
     search_chunk already wrote the data to inode_addr, so we can just exit.
     Otherwise, the for loop will continue with the search. */
  if (search_two_indirect_zone(
        mfs, 
        cur_inode->two_indirect, 
        inode_addr, 
        name)){
    return true;
  }

  return false;
}

/* Searches a zone for a directory entry with a given name, and updates an inode
 * address with the address of the inode that corresponds to that name. 
 * @param mfs MinixFileSystem struct that holds the current filesystem and some
 *  useful information.
 * @param zone_num the zone number to search.
 * @param inode_addr a poninter to a variable that holds the address of the real
 *  inode on the image. 
 * @param name the name of the directory entry we are looking for in the current
 *  inode.
 * @return bool true if we found a valid directory entry, false otherwise.
 */
bool search_zone(
    min_fs* mfs, 
    uint32_t zone_num,
    uint32_t* inode_addr, 
    char* name) {
  int i;

  /* Skip over the indirect zone if it is not used. */
  if (zone_num == 0) {
    return false;
  }

  /* The actual address of the zone. */
  uint32_t zone_addr = mfs->partition_start + (zone_num * mfs->zone_size);

  /* Read all directory entries in this chunk. */
  int num_entries = mfs->zone_size / DIR_ENTRY_SIZE;

  for(i = 0; i < num_entries; i++) {
    min_dir_entry entry;

    /* Seek to the directory entry */
    if (fseek(mfs->file, zone_addr + (i * DIR_ENTRY_SIZE), SEEK_SET) == -1) {
      fprintf(stderr, "error seeking to directory entry: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Read the directory entry */
    if(fread(&entry, DIR_ENTRY_SIZE, 1, mfs->file) < 1) {
      fprintf(stderr, "error reading directory entry: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Check if entry is valid (inode != 0) and name matches */
    if(entry.inode != 0 && strcmp((char*)entry.name, name) == 0) {
      /* Get the address of the inode address that we found. (Go back one inode
         size because we just read it). */
      *inode_addr = mfs->b_inodes + ((entry.inode - 1) * INODE_SIZE);
      return true;
    }
  }

  /* There was no directory entry that had this filename. */
  return false;
}

/* Searches the zones that the indirect zone holds for an entry with a 
 * corresponding name. If a name is found (and it is not deleted) 
 * populate the inode_addr with the "real" address of the found inode, and 
 * return true. Otherwise, return false.
 * @param mfs MinixFileSystem struct that holds the current filesystem and some
 *  useful information.
 * @param zone_num the zone number to search.
 * @param inode_addr a poninter to a variable that holds the address of the real
 *  inode on the image. 
 * @param name the name of the directory entry we are looking for in the current
 *  inode.
 * @return bool true if we found a valid directory entry, false otherwise.
 */
bool search_indirect_zone(
    min_fs* mfs, 
    uint32_t zone_num,
    uint32_t* inode_addr, 
    char* name) {
  int i;

  /* Skip over the indirect zone if it is not used. */
  if (zone_num == 0) {
    return false;
  }

  /* Start reading the first block in that indirect zone. */
  if (fseek(
        mfs->file, 
        mfs->partition_start+(zone_num*mfs->zone_size), 
        SEEK_SET) == -1) {
    fprintf(stderr, "error seeking to indirect zone.\n");
    exit(EXIT_FAILURE);
  }

  /* How many zone numbers we are going to read (how many fit in the first block
     of the indirect zone) */
  int total_indirect_inodes = mfs->sb.blocksize / sizeof(uint32_t);

  /* For every zone number in that first indirect inode block. */
  for(i = 0; i < total_indirect_inodes; i++) {
    /* The zone number that holds directory entries. */
    uint32_t indirect_zone_number;
   
    /* Read the number that holds the zone number. */
    if(fread(&indirect_zone_number, sizeof(uint32_t), 1, mfs->file) < 1) {
      fprintf(stderr, "error reading indirect zone: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Search the direct zone for an entry. If found, then we know that 
       search_chunk already wrote the data to inode_addr, so we can just exit.
       Otherwise, the for loop will continue with the search. */
    if (search_zone(mfs, indirect_zone_number, inode_addr, name)) {
      return true;
    }
  }

  return false;
}

/* Searches the zones that the double indirect zone holds for an entry with a 
 * corresponding name. If a name is found (and it is not deleted) 
 * populate the inode_addr with the "real" address of the found inode, and 
 * return true. Otherwise, return false.
 * @param mfs MinixFileSystem struct that holds the current filesystem and some
 *  useful information.
 * @param zone_num the zone number to search.
 * @param inode_addr a poninter to a variable that holds the address of the real
 *  inode on the image. 
 * @param name the name of the directory entry we are looking for in the current
 *  inode.
 * @return bool true if we found a valid directory entry, false otherwise.
 */
bool search_two_indirect_zone(
    min_fs* mfs, 
    uint32_t zone_num,
    uint32_t* inode_addr, 
    char* name) {
  int i;

  /* Skip over the indirect zone if it is not used. */
  if (zone_num == 0) {
    return false;
  }

  /* Start reading the first block in the double indirect zone. */
  if (fseek(
        mfs->file, 
        mfs->partition_start + (zone_num*mfs->zone_size), 
        SEEK_SET) == -1) {
    fprintf(stderr, "error seeking to double indirect zone.\n");
    exit(EXIT_FAILURE);
  }

  /* How many zone numbers we are going to read (how many fit in the first block
     of the indirect zone) */
  int total_indirect_inodes = mfs->sb.blocksize / sizeof(uint32_t);

  /* For every zone number in that first double indirect inode block. */
  for(i = 0; i < total_indirect_inodes; i++) {
    /* The zone number that holds the indierct zone numbers. */
    uint32_t two_indirect_zone_number;
   
    /* Read the number that holds the zone number. */
    if(fread(&two_indirect_zone_number, sizeof(uint32_t), 1, mfs->file) < 1) {
      fprintf(stderr, "error reading double indirect zone number: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Search the indirect zone for an entry. If found, then we know that 
       search_chunk already wrote the data to inode_addr, so we can just exit.
       Otherwise, the for loop will continue with the search. */
    if (search_indirect_zone(mfs, two_indirect_zone_number, inode_addr, name)) {
      return true;
    }
  }

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
  int16_t log_zone_size = sb->log_zone_size; /* log_zone_size? */
  uint16_t zonesize = blocksize << log_zone_size;

  return zonesize;
}

