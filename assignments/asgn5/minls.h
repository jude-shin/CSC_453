#ifndef MINLS
#define MINLS 

#include "disk.h"

/* Prints some information about an inode including the rwx permissions for the
   Group, User, and Other, it's size, and it's name. */
void ls_file(FILE* s, min_inode* inode, unsigned char* name);

/* Prints the directory elements to stream s. 
   Conforms to the block_proc function and can be used as a callback. */
bool ls_block(
    FILE* s, 
    min_fs* mfs, 
    min_inode* inode,
    uint32_t zone_num, 
    uint32_t block_num, 
    void* context);

/* Prints every directory entry in a directory. */
void ls_directory(FILE* s, min_fs* mfs, min_inode* inode, char* can_path);

#endif
