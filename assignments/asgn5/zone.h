#ifndef ZONE
#define ZONE

#include <stdbool.h>
#include <stdio.h>
#include "disk.h"

/* Callback function type for processing blocks. 
 * Returns true if processing should stop early, false to continue. */
typedef bool (*block_processor_t)(
    FILE* s,            /* I/O stream */
    min_fs* mfs,        /* minix filesystem structure */ 
    min_inode* inode,   /* current inode of interest */
    uint32_t zone_num,  /* zone number of interest */
    uint32_t block_num, /* block number of interest */
    void* context);     /* a pointer to any context that might be needed */

/* Callback function type for processing a hole
 * Returns true if processing should stop early, false to continue. */
typedef bool (*hole_processor_t)(
    FILE* s,              /* Output stream */
    min_inode* inode,     /* current inode of interest */ 
    uint32_t hs,          /* hole size */
    uint32_t* bytes_read);/* bytes read to this point. */

/* ============== */
/* ZONE TRAVERSAL */
/* ============== */
/* Process all blocks in a direct zone */
bool process_direct_zone(
    FILE* s, 
    min_fs* mfs, 
    min_inode* inode,
    uint32_t zone_num, 
    block_processor_t block_proc,
    hole_processor_t hole_proc,
    void* context);

/* Process all zones pointed to by an indirect zone */
bool process_indirect_zone(
    FILE* s, 
    min_fs* mfs, 
    min_inode* inode,
    uint32_t zone_num, 
    block_processor_t block_proc,
    hole_processor_t hole_proc,
    void* context);

/* Process all zones pointed to by a double indirect zone */
bool process_two_indirect_zone(
    FILE* s, 
    min_fs* mfs, 
    min_inode* inode,
    uint32_t zone_num, 
    block_processor_t block_proc,
    hole_processor_t hole_proc,
    void* context);

#endif 
