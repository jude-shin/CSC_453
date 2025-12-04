#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include "disk.h"
#include "print.h"
#include "zone.h"

/*==========*/
/* BASIC IO */
/*==========*/
/* Opens the img as readonly, and sets up datastructures like the mfs
 * context. If anything goes wrong, then this function will exit with 
 * EXIT_FAILURE. 
 * @param mfs a struct that holds an open file descriptor, and the offset (the
 *  offset from the beginning of the image which indicates the beginning of the
 *  partition that holds the filesystem.
 * @param img_path the host path to the image that we are looking at.
 * @param p_prt what partition number we are interested in. -1 means that
 *  this image is unpartitioned.
 * @param s_prt what subpartition number we are interested in. -1 means that
 *  there are no subpartitions. 
 * @param verb if this is true, print the partition table's contents
 * @return void. 
 */
void open_mfs(
    min_fs* mfs, 
    char* img_path, 
    int p_prt, 
    int s_prt,
    bool verb) {
  /* How far from the beginning of the disk the filesystem resides. */
  uint32_t offset;

  /* The (sub)partition table that is read from the image. */
  min_part_tbl pt, spt;

  /* The FILE that the image resides in. */
  FILE* img;

  /* Offset should start at 0. */
  offset = 0;

  /* Open the file for reading only */
  img = fopen(img_path, "rb");
  if (img == 0) {
    fprintf(stderr, "error opening %s: %d\n", img_path, errno);
    exit(EXIT_FAILURE);
  }

  /* Seek to the primary partition if one was provided, otherwise treat the 
     image as unpartitioned (offset of 0). */
  if (p_prt != -1) {
    /* Populate the partition table (pt) based on what partition was chosen. */
    load_part_table(&pt, (p_prt*sizeof(min_part_tbl))+PART_TABLE_OFFSET, img);

    /* Check system type. */
    if (pt.type != MINIX_PARTITION_TYPE) {
      fprintf(stderr, "This is not a minix image.\n");
      exit(EXIT_FAILURE);
    }

    /* Print the contents if we are in verbose mode. */
    if (verb) {
      print_part_table(stderr, &pt);
    }

    /* Validate the two signatures in the beginning of the disk. */
    if (!validate_signatures(img, offset)) {
      fprintf(stderr, "primary partition table signatures are not valid\n");
      exit(EXIT_FAILURE);
    }

    /* Calculate where the important part of the partition is. */
    offset = pt.lFirst*SECTOR_SIZE;

    /* Seek to the subpartition */
    if (s_prt != -1) { 
      /* Populate the subpartition table (spt) based on what subpartition was 
         chosen. */
      load_part_table(
          &spt, 
          offset+(s_prt*sizeof(min_part_tbl))+PART_TABLE_OFFSET, 
          img);

      /* Validate the two signatures in the partition. */
      if (!validate_signatures(img, offset)) {
        fprintf(stderr, "subpartition table signatures are not valid\n");
        exit(EXIT_FAILURE);
      }

      /* Check system type. */
      if (spt.type != MINIX_PARTITION_TYPE) {
        fprintf(stderr, "This is not a minix image.\n");
        exit(EXIT_FAILURE);
      }

      /* Print the contents if we are in verbose mode. */
      if (verb) {
        fprintf(stderr, "(sub)");
        print_part_table(stderr, &pt);
      }

      /* Calculate where the important part of the partition is. */
      offset = spt.lFirst*SECTOR_SIZE;
    }
  }

  /* Update the struct to reflect the file descriptor and offset found. */
  mfs->file = img;
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

  /* Print the contents if we are in verbose mode. */
  if (verb) {
    print_superblock(stderr, &mfs->sb);
  }

  /* Update the mfs context to store the zone size of this filesystem. */
  mfs->zone_size = get_zone_size(&mfs->sb);

  /* Get the number of directories in a block */
  mfs->num_dir_p_block = mfs->sb.blocksize / DIR_ENTRY_SIZE;

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

/* Closes the file descriptor for the file in the min_fs struct given. (Along
 * with anything else you might need to clean up).
 * @param mfs MinixFileSystem struct that holds the current filesystem. 
 * @return void.
 */
void close_mfs(min_fs* mfs) {
  if (fclose(mfs->file) == -1) {
    fprintf(stderr, "Error close(2)ing the img: %d", errno);
    exit(EXIT_FAILURE);
  }
}


/*============*/
/* VALIDATION */
/*============*/

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
 * @param pt the partitinon tbl struct that will be filled (& referenced later).
 * @param addr the address that the partition will start at. 
 * @param image the imagefile that the disk is stored in.
 * @return void.
 */
void load_part_table(min_part_tbl* pt, uint32_t addr, FILE* img) { 
  /* Seek to the correct location that the partition table resides. */
  if (fseek(img, addr, SEEK_SET)) {
    fprintf(stderr, "error seeking to partition table: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  /* Read the partition table, storing its contents in the struct for us to 
     reference later on. */
  if (fread(pt, sizeof(min_part_tbl), 1, img) < 1) {
    fprintf(stderr, "error reading partition table: %d\n", errno);
    exit(EXIT_FAILURE);
  }
}

/* TODO: remove duplicate functionality? */
/* Fills a superblock based ona minix filesystem (a image and an offset)
 * @param mfs a struct that holds information; most importantly, the struct for
 *  the superblock.
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
    min_inode* inode,
    char* path,
    char* can_minix_path,
    unsigned char* cur_name) {

  /* The tokenized next directory entry name that we are looking for. */
  char* token = strtok(path, DELIMITER);

  /* The current inode we are processing. */
  min_inode cur_inode;

  /* Seek the read head to the first inode. */
  if (fseek(mfs->file, mfs->b_inodes, SEEK_SET) == -1) {
    fprintf(stderr, "error seeking to the root inode: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  /* Read the value at that address into the root inode struct. */
  if (fread(&cur_inode, sizeof(min_inode), 1, mfs->file) < 1) {
    fprintf(stderr, "error reading the root inode: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  /* Parse all of the directories that the user gave by traversing through the
     directories till we are at the last inode. */
  while(token != NULL) {
    /* Length of the next "path" */
    size_t len = strlen(token); /* fn doesn't indicate error. */
 
    /* Make sure that there isn't a filename with a length that is greater than
       DIR_NAME_SIZE*/
    if (len > DIR_NAME_SIZE) {
      fprintf(stderr, "encountered a name greater than %d!\n", DIR_NAME_SIZE);
      exit(EXIT_FAILURE);
    }

    /* Copy the string name to cur_name so we can keep track of the last
       processed name. */
    if (cur_name != NULL) {
      /* Just make sure you don't go over the max length. */
      if (len >= DIR_NAME_SIZE) {
        len = DIR_NAME_SIZE - 1;
      }
      memcpy(cur_name, token, len); /* fn doesn't indicate error. */
      cur_name[len] = '\0';
    }

    /* Add the token (and a delimiter) to the build canonicalized minix path. */
    if (can_minix_path != NULL) {
      strcat(can_minix_path, token); /* fn doesn't indicate error. */
      strcat(can_minix_path, DELIMITER); /* fn doesn't indicate error. */
    }

    /* The current inode must be traversable (a directory) */
    if (!(cur_inode.mode & DIR_FT)) {
      fprintf(
          stderr, 
          "error traversing the path. %s is not a directory!\n", 
          token);
      return false;
    }

    /* Search through the direct, indirect, and double indirect zones for a 
       directory entry with a matching name. */
    if (search_all_zones(mfs, inode, &cur_inode, token)) {
      cur_inode = *inode;
      token = strtok(NULL, DELIMITER); /* fn doesn't indicate error. */
    }
    else {
      fprintf(stderr, "error traversing the path: directory not found!\n");
      return false;
    }
  }

  /* Update the value of inode with the one that we just found. */
  *inode = cur_inode;
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
 * @param inode the inode that will be populated when found.
 * @param cur_inode the inode (which is a directory) that we are searching in.
 *  inode on the image. 
 * @param name the name of the directory entry we are looking for in the current
 *  inode.
 * @return bool true if we found a valid directory entry, false otherwise.
 */
bool search_all_zones(
    min_fs* mfs, 
    min_inode* inode, 
    min_inode* cur_inode,
    char* name) {

  /* Linear search the direct zones. */
  int i;
  for(i = 0; i < DIRECT_ZONES; i++) {
    /* Search the direct zone for an entry. If found, then we know that 
       search_block already wrote the data to inode_addr, so we can just exit.
       Otherwise, the for loop will continue with the search. */
    if (process_direct_zone(
          NULL, /* We dont need to print to a stream. */
          mfs, 
          inode, 
          cur_inode->zone[i], 
          search_block, /* This is the search function. */
          NULL, /* If we see a hole we don't have to do anything special. */
          (void*)name)) {
      return true;
    }
  }

  /* Search the indirect zone for an entry. If found, then we know that 
     search_block already wrote the data to inode_addr, so we can just exit.
     Otherwise, the for loop will continue with the search. */
  if (process_indirect_zone(
        NULL, /* We dont need to print to a stream. */
        mfs, 
        inode, 
        cur_inode->indirect, 
        search_block, /* This is the search function. */
        NULL, /* If we see a hole we don't have to do anything special. */
        (void*)name)) {
    return true;
  }

  /* Search the double indirect zone for an entry. If found, then we know that 
     search_block already wrote the data to inode_addr, so we can just exit.
     Otherwise, the for loop will continue with the search. */
  if (process_two_indirect_zone(
        NULL, /* We dont need to print to a stream. */
        mfs, 
        inode, 
        cur_inode->two_indirect, 
        search_block, /* This is the search function. */
        NULL, /* If we see a hole we don't have to do anything special. */
        (void*)name)){
    return true;
  }

  return false;
}

/* Searches a block to find a directory entry with a mathcing name. 
 * Conforms to the block_processor function and can be used as a callback.
 * @param s Does nothing.
 * @param mfs a struct that holds an open file descriptor, and the offset (the
 *  offset from the beginning of the image which indicates the beginning of the
 *  partition that holds the filesystem.
 * @param inode the inode that will be populated when found.
 * @param zone_num the zone number containing this block.
 * @param block_num the block number with in the zone.
 * @return bool if the inode was found in this block or not.
 */
bool search_block(
    FILE* s, /* We dont need to have any stream for the output. */
    min_fs* mfs, 
    min_inode* inode, 
    uint32_t zone_num,
    uint32_t block_num,
    void* name) {
  /* The address of the block of interest on the minix image. */
  uint32_t block_addr = get_block_addr(mfs, zone_num, block_num);

  int i;
  /* Loop through every directory entry in this block. */
  for (i = 0; i < mfs->num_dir_p_block; i++) {
    /* The directory entry that we are currently on. */
    min_dir_entry entry;

    /* Seek to the directory entry */
    if (fseek(mfs->file, block_addr + (i * DIR_ENTRY_SIZE), SEEK_SET)) {
      fprintf(stderr, "error seeking to directory entry: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Read the directory entry */
    if(fread(&entry, DIR_ENTRY_SIZE, 1, mfs->file) < 1) {
      fprintf(stderr, "error reading directory entry: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Check if entry is valid (inode != 0) and name matches */
    /* strcmp fn doesn't indicat errors.  */
    if(entry.inode != 0 && strcmp((char*)entry.name, (char*)name) == 0) {
      /* Get the address of the inode address that we found. (Go back one inode
         size because we just read it). */
      uint32_t inode_addr = mfs->b_inodes + ((entry.inode - 1) * INODE_SIZE);

      /* "Duplicate" the inode information to the inode in the main function so
         we can reference it later. */
      duplicate_inode(mfs, inode_addr, inode);

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
 * @return uint32_t the size of a zone
 */
uint32_t get_zone_size(min_superblock* sb) {
  uint32_t blocksize = sb->blocksize;
  int16_t log_zone_size = sb->log_zone_size;
  uint32_t zonesize = blocksize << log_zone_size;

  return zonesize;
}

/* Calculates the zone address on a minix filesystem.
 * @param mfs a struct that holds info on the minix image.  
 * @param zone_num the zone number containing this block.
 * @return uint32_t the address on the minix image
 */
uint32_t get_zone_addr(min_fs* mfs, uint32_t zone_num) {
  return mfs->partition_start + (zone_num * mfs->zone_size);
}

/* Calculates the block address on a minix filesystem.
 * @param mfs a struct that holds info on the minix image.  
 * @param zone_num the zone number containing this block.
 * @param block_num the block number with in the zone.
 * @return uint32_t the address on the minix image
 */
uint32_t get_block_addr(min_fs* mfs, uint32_t zone_num, uint32_t block_num) {
  return get_zone_addr(mfs, zone_num) + (block_num * mfs->sb.blocksize);
}
