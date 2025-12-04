#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "zone.h"
#include "disk.h"

/* Process all blocks in a direct zone using the block and hole processor 
 * callbacks.
 * @param s I/O stream
 * @param mfs the minix filesystem structure
 * @param inode current inode of interest
 * @param zone_num zone number of interest
 * @param block_proc the callback fn used when a block is processed
 * @param hole_proc the callback fn used when a hole is processed
 *  (if NULL, do nothing)
 * @param context any context that might be needed for the callback 
 * @returns true if processing should stop early, false to continue.
 */
bool process_direct_zone(
    FILE* s, 
    min_fs* mfs, 
    min_inode* inode,
    uint32_t zone_num, 
    block_processor_t block_proc,
    hole_processor_t hole_proc,
    void* context) {

  /* If there is a hole encountered */
  if (zone_num == 0) {
    /* Only process if there is a hole process callback. */
    if (hole_proc == NULL) {
      return false;
    }
    else {
      /* Only do the callback if the hole_proc callback is present */
      /* Fill a full data zone size worth of zeros. */
      uint32_t hs = mfs->zone_size;
      return hole_proc(s, inode, hs, context);
    }
  }

  /* Process all blocks in this zone */
  uint32_t num_blocks = mfs->zone_size / mfs->sb.blocksize;

  int i;
  for (i = 0; i < num_blocks; i++) {
    if (block_proc(s, mfs, inode, zone_num, i, context)) {
      return true;
    }
  }

  return false;
}

/* Process all blocks in an indirect zone using the block and hole processor 
 * callbacks.
 * @param s I/O stream
 * @param mfs the minix filesystem structure
 * @param inode current inode of interest
 * @param zone_num zone number of interest
 * @param block_proc the callback fn used when a block is processed
 * @param hole_proc the callback fn used when a hole is processed
 *  (if NULL, do nothing)
 * @param context any context that might be needed for the callback 
 * @returns true if processing should stop early, false to continue.
 */
bool process_indirect_zone(
    FILE* s, 
    min_fs* mfs, 
    min_inode* inode,
    uint32_t zone_num, 
    block_processor_t block_proc,
    hole_processor_t hole_proc,
    void* context) {

  /* How many zone numbers fit in the first block */
  uint32_t num_indirect = mfs->sb.blocksize / sizeof(uint32_t);

  /* If there is a hole encountered */
  if (zone_num == 0) {
    /* Only do the callback if the hole_proc callback is present */
    if (hole_proc == NULL) {
      return false;
    }
    else {
      /* Fill the indirect hole. */
      uint32_t hs = mfs->sb.blocksize*num_indirect;
      return hole_proc(s, inode, hs, context);
    }
  }

  /* The first block is at the address of that zone */
  uint32_t block_addr = mfs->partition_start + (zone_num * mfs->zone_size);

  /* For every zone number in the indirect block */
  int i;
  for (i = 0; i < num_indirect; i++) {
    uint32_t indirect_zone_num;

    /* Seek to the zone number entry */
    if (fseek(mfs->file, block_addr + (i * sizeof(uint32_t)), SEEK_SET) == -1) {
      fprintf(stderr, "error seeking to indirect zone: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Read the zone number */
    if (fread(&indirect_zone_num, sizeof(uint32_t), 1, mfs->file) < 1) {
      fprintf(stderr, "error reading indirect zone: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* If there is a hole encountered */
    if (indirect_zone_num == 0) {
      /* Fill the hole if there is callback to handle it. */
      uint32_t hs = mfs->sb.blocksize;
      if (hole_proc != NULL && hole_proc(s, inode, hs, context)) {
        return true;
      }
      continue;
    }

    /* Process that direct zone */
    if (process_direct_zone(
          s, 
          mfs, 
          inode, 
          indirect_zone_num, 
          block_proc, 
          hole_proc, 
          context)) {
      return true;
    }
  }

  return false;
}

/* Process all blocks in a double indirect zone using the block and hole 
 * processor callbacks.
 * @param s I/O stream
 * @param mfs the minix filesystem structure
 * @param inode current inode of interest
 * @param zone_num zone number of interest
 * @param block_proc the callback fn used when a block is processed
 * @param hole_proc the callback fn used when a hole is processed
 *  (if NULL, do nothing)
 * @param context any context that might be needed for the callback 
 * @returns true if processing should stop early, false to continue.
 */
bool process_two_indirect_zone(
    FILE* s, 
    min_fs* mfs, 
    min_inode* inode,
    uint32_t zone_num,
    block_processor_t block_proc,
    hole_processor_t hole_proc,
    void* context) {

  /* How many zone numbers fit in a block */
  uint32_t num_indirect = mfs->sb.blocksize / sizeof(uint32_t);

  /* If there is a hole encountered */
  if (zone_num == 0) {
    /* Only do the callback if the hole_proc callback is present */
    if (hole_proc == NULL) {
      return false;
    }
    else {
      /* Fil the hole. */
      uint32_t hs = mfs->sb.blocksize * num_indirect * num_indirect;
      return hole_proc(s, inode, hs, context);
    }
  }

  /* The address in the minix image. */
  uint32_t zone_addr = mfs->partition_start + (zone_num * mfs->zone_size);

  /* For every zone number in the double indirect block */
  int i;
  for (i = 0; i < num_indirect; i++) {
    uint32_t indirect_zone_num;

    /* Seek to the zone number entry */
    if (fseek(mfs->file, zone_addr + (i * sizeof(uint32_t)), SEEK_SET) == -1) {
      fprintf(stderr, "error seeking to double indirect zone: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* Read the zone number */
    if (fread(&indirect_zone_num, sizeof(uint32_t), 1, mfs->file) < 1) {
      fprintf(stderr, "error reading double indirect zone number: %d\n", errno);
      exit(EXIT_FAILURE);
    }

    /* If there is a hole encountered */
    if (indirect_zone_num == 0) {
      /* Only fill the hole if hole_proc callback is present */
      uint32_t hs = mfs->sb.blocksize * num_indirect;
      if (hole_proc != NULL && hole_proc(s, inode, hs, context)) {
        return true;
      }
      continue;
    }

    /* Process that indirect zone */
    if (process_indirect_zone(
          s, 
          mfs, 
          inode, 
          indirect_zone_num,
          block_proc, 
          hole_proc, 
          context)) {
      return true;
    }
  }

  return false;
}
