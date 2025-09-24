#include <unistd.h> 
#include <stddef.h> 

#include "datastructures.h"

#define HUNK_SIZE 64000000
#define CHUNK_SIZE 64000

static void *head_addr = NULL;
static void *tail_addr = NULL;


// CHunk functions
/*
Checks the current and next chunks, to clean up space.  If curr chunk is not being used, and next chunk is also not being used, keep the curr header, and turn the rest of space into the data of the curr
header. 
Repeat recursively (setting the prev header as curr in this function) 
until the end is reached, or there is a chunk that is being used.
return the address of the header whether it merged or not
*/
void *merge_next(Chunk *curr) {
  return NULL;
}

/*
Checks the current and prev chunks, to clean up space.
If curr chunk is not being used, and prev chunk is also not being used, 
keep the prev header, and turn the rest of space into the data of the prev 
header. 
Repeat recursively (setting the prev header as curr in this function)
until the end is reached, or there is a chunk that is being used.
return the address of the header whether it merged or not
*/
void *merge_prev(Chunk *curr) {
  return NULL;
}


// Hunk functions

void *get_head() {
  if (head_addr == NULL) {
    // this is the first time that I have called malloc, and I need to get
    // the original break point
    // this will also be the header of the linked list
    head_addr = sbrk(HUNK_SIZE);
    tail_addr = head_addr; 
  }
  return head_addr;
}
