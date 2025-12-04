#ifndef MINLS
#define MINLS 

#include "disk.h"

/* Prints some information about an inode including the rwx permissions for the
   Group, User, and Other, it's size, and it's name. */
void ls_file(FILE* s, min_inode* inode, unsigned char* name);

/* TODO: */
void ls_files_in_block(
    FILE* s, 
    min_fs* mfs, 
    uint32_t zone_num, 
    uint32_t block_number);


/* Given a zone, print all of the files that are on that zone if they are
   valid. */
void ls_files_in_direct_zone(FILE* s, min_fs* mfs, uint32_t zone_num);

/* Given an indirect zone, print all of the files that are on the zones that
   the indirect zone points to if they are valid. */
void ls_files_in_indirect_zone(FILE* s, min_fs* mfs, uint32_t zone_num);

/* Given a double indirect zone, print all of the files that are on the zones 
   that are on the zones that the indirect zone points to if they are valid
   (double indirect -> indirect -> zone -> file). */
void ls_files_in_two_indirect_zone(FILE* s, min_fs* mfs, uint32_t zone_num);

/* Prints every directory entry in a directory. */
void ls_directory(FILE* s, min_fs* mfs, min_inode* inode, char* can_path);

#endif
