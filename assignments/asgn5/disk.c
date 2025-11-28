#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "disk.h"

#define SECTOR_SIZE 512 /* the sector size for a minix fs is 512 bytes. */

/* the address where the partition table is. */
#define PART_TABLE_ADDR 0x1BE 
/* the partition type that indicates this is a minix system */
#define MINIX_PARTITION_TYPE 0x81
/* bootind will be equal to this macro if the partition is bootable */
#define BOOTABLE_MAGIC 0x80

/* byte addresses & the expected values for verifying a partition signature. */
#define SIG510_OFFSET 510 
#define SIG510_EXPECTED 0x55 
#define SIG511_OFFSET 511
#define SIG511_EXPECTED 0xAA

/* Based on an address (primary partition will start at 0, and any subpartition
 * will start somewhere else) this function populates the given partition_table
 * struct with the data read in the image. Whether the populated data is valid 
 * is entirely up to the address variable.
 * @param partition_table the struct that will be filled (& referenced later).
 * @param addr the address that the partition will start at. 
 * @param image the imagefile that the disk is stored in.
 * @return void.
 */
void load_part_table(part_tbl* partition_table, long addr, FILE* image) { 
  /* Seek to the correct location that the partition table resides. */
  fseek(image, addr, SEEK_SET);
  
  /* Read the partition_table, storing its contents in the struct for us to 
     reference later on. */
  ssize_t bytes = fread(partition_table, sizeof(part_tbl), 1, image);
  if (bytes <= sizeof(part_tbl)) {
    fprintf(stderr, "error reading partition table: %d\n", errno);
    exit(EXIT_FAILURE);
  }
}

/* Check to see if the partition table holds useful information for this
 * assignment. This includes the two signatures, that this is a bootable image,
 * and if the partition is from minix. This function does not return anything;
 * If the program does not exit after calling this function, then the partition
 * table is valid. Otherwise, it will just exit and do nothing.
 * @param partition_table the struct that holds all this information. 
 * @return void.
 */
void validate_part_table(part_tbl* partition_table) {
  /* check that the image is bootable */
  /* TODO: we might not actually want this. */
  if (partition_table->bootind != BOOTABLE_MAGIC) {
    fprintf(stderr, "this image is not bootable\n");
    exit(EXIT_FAILURE);
  }

  /* check that the partition type is of minix. */
  if (partition_table->type != MINIX_PARTITION_TYPE) {
    fprintf(stderr, "this is not a minix image\n");
    exit(EXIT_FAILURE);
  }

  /* Check the first signature. 510 bytes from the beginning of the 
     partition table. */
  unsigned char sig510 = *((unsigned char*)partition_table+SIG510_OFFSET);
  fprintf(stderr, "SIG510: %c\n", sig510);
  if (sig510 != SIG510_EXPECTED) {
    fprintf(stderr, "first signature could not be validated.\n");
    exit(EXIT_FAILURE);
  }

  /* Check the first signature. 511 bytes from the beginning of the 
     partition table. */
  unsigned char sig511 = *((unsigned char*)partition_table+SIG511_OFFSET);
  fprintf(stderr, "SIG511: %c\n", sig511);
  if (sig511 != SIG511_EXPECTED) {
    fprintf(stderr, "second signature could not be validated.\n");
    exit(EXIT_FAILURE);
  }
}
  

/* Opens the imagefile as readonly, and calculates the offset of the specified
 * partition. If anything goes wrong, then this function will exit with 
 * EXIT_FAILURE. 
 * @param mfs a struct that holds an open file descriptor, and the offset (the
 *  offset from the beginning of the image which indicates the beginning of the
 *  partition that holds the filesystem.
 * @param imagefile_path the host path to the image that we are looking at.
 * @param prim_part what partition number we are interested in. -1 means that
 *  this image is unpartitioned.
 * @param sub_part what subpartition number we are interested in. -1 means that
 *  there are no subpartitions. 
 * @return void. 
 */
void open_mfs(min_fs* mfs, char* imagefile_path, int prim_part, int sub_part) {
  FILE* imagefile = fopen(imagefile_path, "r");
  if (imagefile == 0) {
    fprintf(stderr, "error opening %s: %d\n", imagefile_path, errno);
    exit(EXIT_FAILURE);
  }

  /* How far from the beginning of the disk the filesystem resides. */
  long offset = 0;

  /* The partition_table that is read from the image. */
  part_tbl pt = {0};
 
  /* Populate the partition table (pt) */
  load_part_table(&pt, PART_TABLE_ADDR, imagefile);

  /* Check the signatures, system type, and if this is bootable. */
  validate_part_table(&pt);

  /* Seek to the primary partition */
  /* Treat the primary partition as unpartitioned. */
  if (prim_part != -1) {
    offset += prim_part * (pt.size*SECTOR_SIZE);
  }
  offset += pt.lFirst;

  /* Seek to the subpartition */
  /* Treat the subpartition as unpartitioned. */
  if (sub_part != -1) { 
    /* The subpartition table that is read from the image. */
    part_tbl spt = {0};
  
    /* Populate the subpartition table (spt). */
    load_part_table(&spt, offset+PART_TABLE_ADDR, imagefile);

    /* Check the signatures, system type, and if this is bootable. */
    validate_part_table(&spt);

    /* If there is a subpartition, then this is the final address! */
    offset = spt.lFirst;
  }

  /* Update the struct to reflect the file descriptor and offset found. */
  mfs->file = imagefile;
  mfs->partition_start = offset;
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
