#ifndef MINGET 
#define MINGET 

#include "disk.h"

/* Prints the contents of a regular file to the stream s given an inode. */
void get_file_contents(FILE* s, min_fs* mfs, min_inode* inode);

/* Reads the contents of the block to stream s.
   Conforms to the block_proc function and can be used as a callback. */
bool get_block_contents(
    FILE* s, 
    min_fs* mfs, 
    min_inode* inode, 
    uint32_t zone_num, 
    uint32_t block_number, 
    void* bytes_read);

/* Fills a hole with zeros. Conforms to the hole_proc function that is used
   as callbacks.  */
bool fill_hole(
    FILE* s, 
    min_inode* inode, 
    uint32_t hs,
    uint32_t* bytes_read);

#endif
