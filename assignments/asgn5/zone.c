#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "zone.h"
#include "disk.h"

/* Process all blocks in a direct zone using the provided callback */
bool process_direct_zone(FILE* s, min_fs* mfs, min_inode* inode,
    uint32_t zone_num, block_processor_t processor,
    void* context) {
  /* Skip holes - let caller handle them */
  if (zone_num == 0) {
    return false;
  }

  /* Process all blocks in this zone */
  uint32_t num_blocks = mfs->zone_size / mfs->sb.blocksize;

  for (int i = 0; i < num_blocks; i++) {
    if (processor(s, mfs, inode, zone_num, i, context)) {
      return true;
    }
  }

  return false;
}

/* Process all zones pointed to by an indirect zone */
bool process_indirect_zone(FILE* s, min_fs* mfs, min_inode* inode,
    uint32_t zone_num, block_processor_t processor,
    void* context) {
  /* Skip holes - let caller handle them */
  if (zone_num == 0) {
    return false;
  }

  /* The first block is at the address of that zone */
  uint32_t block_addr = mfs->partition_start + (zone_num * mfs->zone_size);

  /* How many zone numbers fit in the first block */
  uint32_t num_indirect = mfs->sb.blocksize / sizeof(uint32_t);

  /* For every zone number in the indirect block */
  for (int i = 0; i < num_indirect; i++) {
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

    /* Process that direct zone */
    if (process_direct_zone(s, mfs, inode, indirect_zone_num, processor, 
          context)) {
      return true;
    }
  }

  return false;
}

/* Process all zones pointed to by a double indirect zone */
bool process_two_indirect_zone(FILE* s, min_fs* mfs, min_inode* inode,
    uint32_t zone_num, block_processor_t processor,
    void* context) {
  /* Skip holes - let caller handle them */
  if (zone_num == 0) {
    return false;
  }

  /* How many zone numbers fit in a block */
  uint32_t num_indirect = mfs->sb.blocksize / sizeof(uint32_t);

  uint32_t zone_addr = mfs->partition_start + (zone_num * mfs->zone_size);

  /* For every zone number in the double indirect block */
  for (int i = 0; i < num_indirect; i++) {
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

    /* Process that indirect zone */
    if (process_indirect_zone(s, mfs, inode, indirect_zone_num, processor, 
          context)) {
      return true;
    }
  }

  return false;
}
