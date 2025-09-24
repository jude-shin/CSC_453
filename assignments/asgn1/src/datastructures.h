#ifndef DATASTRUCTURES 
#define DATASTRUCTURES 

#include <stdbool.h>
// hold the data structure for the linked list in here. this should hold some
// pointer information, size, whether it is free or being used, and possibly
// the previous chunk as well

/*
A "chunk" is a header and a data portion of the overall "hunk" that 
sbrk2 gave us in the beginning.
This follows a doubly linked list data structure with some extra info.
The header holds the following information: 
  How large the usable data segment of this chunk is.
  TODO: If the chunk is currently being used (if it is freed or not).
    why are we doing this?
  A pointer to the previous chunk (which is the same as the beginning of it's 
  header) if there is one.
  A pointer to the next chunk (which is the same as the beginning of it's 
  header) if there is one.
*/
typedef struct Chunk {
  // how large the data segment of this chunk should be
  long unsigned int size;

  // if the data region is being used (TODO: useful for cleanup operations)
  bool is_available;

  // points to the header (chunk) beginnings, not the data portion
  struct Chunk *prev;
  struct Chunk *next;

  // NOTE: where does the data region of this chunk start?
  // we should know this: current chunk pointer plust the size(chunk) should
  // hold the beginning of where the data actually starts
} Chunk;

#endif
