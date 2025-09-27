#ifndef CHUNK
#define CHUNK

#include <stddef.h>

// Size of the hunk's that sbrk() will use
#define HUNK_SIZE 304
#define ALLIGN 16
#define CHUNK_SIZE (sizeof(Chunk)+(ALLIGN-sizeof(Chunk)%ALLIGN))

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
  size_t size;

  // if the data region is being used (TODO: useful for cleanup operations)
  bool is_available;

  // points to the header (chunk) beginnings, not the data portion
  struct Chunk *prev;
  struct Chunk *next;

  // NOTE: where does the data region of this chunk start?
  // we should know this: current chunk pointer plust the size(chunk) should
  // hold the beginning of where the data actually starts
} Chunk;

size_t block_size(size_t size);
Chunk *merge_next(Chunk *curr);
Chunk *merge_prev(Chunk *curr);
Chunk *get_head();
Chunk *find_chunk(Chunk *curr, void *ptr);
Chunk *find_available_chunk(Chunk *curr, size_t size);
Chunk *carve_chunk(Chunk *available_chunk, size_t size, bool initalize);

#endif
