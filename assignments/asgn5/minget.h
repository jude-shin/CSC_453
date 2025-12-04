#ifndef MINGET 
#define MINGET 

#include "disk.h"


/* Prints the contents of a regular file to the stream s given an inode. */
void get_file_contents(FILE* s, min_fs* mfs, min_inode* inode);

bool get_block_contents(
    FILE* s, 
    min_fs* mfs, 
    min_inode* inode, 
    uint32_t zone_num, 
    uint32_t block_number, 
    uint32_t* bytes_read);

/* Prints the contents of a zone to a stream s. */
bool get_direct_zone_contents(
    FILE* s, 
    min_fs* mfs, 
    min_inode* inode, 
    uint32_t zone_num, 
    uint32_t* bytes_read);

/* Prints the contents of an indirect zone to a stream s. */
bool get_indirect_zone_contents(
    FILE* s, 
    min_fs* mfs, 
    min_inode* inode, 
    uint32_t zone_num, 
    uint32_t* bytes_read);

/* Prints the contents of a double indirect zone to a stream s. */
bool get_two_indirect_zone_contents(
    FILE* s, 
    min_fs* mfs, 
    min_inode* inode, 
    uint32_t zone_num, 
    uint32_t* bytes_read);


#endif
