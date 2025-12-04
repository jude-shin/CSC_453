#ifndef ZONE_H
#define ZONE_H

#include <stdbool.h>
#include <stdio.h>
#include "disk.h"

/* Callback function type for processing blocks.
 * Returns true if processing should stop early, false to continue.
 * 
 * Parameters:
 *   s - output stream
 *   mfs - minix filesystem structure
 *   inode - current inode being processed
 *   zone_num - the zone number containing this block
 *   block_num - the block number within the zone
 *   context - user-provided context data
 */
typedef bool (*block_processor_t)(FILE* s, min_fs* mfs, min_inode* inode,
    uint32_t zone_num, uint32_t block_num,
    void* context);

/* Process all blocks in a direct zone */
bool process_direct_zone(FILE* s, min_fs* mfs, min_inode* inode,
    uint32_t zone_num, block_processor_t processor,
    void* context);

/* Process all zones pointed to by an indirect zone */
bool process_indirect_zone(FILE* s, min_fs* mfs, min_inode* inode,
    uint32_t zone_num, block_processor_t processor,
    void* context);

/* Process all zones pointed to by a double indirect zone */
bool process_two_indirect_zone(FILE* s, min_fs* mfs, min_inode* inode,
    uint32_t zone_num, block_processor_t processor,
    void* context);

#endif /* ZONE_H */
