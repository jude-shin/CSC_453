#ifndef CHUNK
#define CHUNK

#include <stddef.h>

// Size of the hunk's that sbrk() will use in bytes.
#define HUNK_SIZE 64000
// Size of our allignment in bytes
#define ALLIGN 16
// Size of our chunk struct in bytes rounded up to a multiple of ALLIGN
#define CHUNK_SIZE (sizeof(Chunk)+(ALLIGN-sizeof(Chunk)%ALLIGN))

// A "chunk" is a header with some information about the hunk of memory we are
// managing. This follows a doubly linked list data structure with some extra 
// information.
typedef struct Chunk {
  // How large the data segment of this chunk should be.
  size_t size;

  // If the data region is being used. (If it is freed or not).
  bool is_available;

  // Points to the header (chunk) beginnings, not the data portion.
  // The prev and next pointers will point to NULL if the current Chunk is the
  // head or the tail respectfully
  struct Chunk *prev;
  struct Chunk *next;

} Chunk;

// 
size_t block_size(size_t size);
Chunk *merge_next(Chunk *curr);
Chunk *merge_prev(Chunk *curr);
Chunk *get_head();
Chunk *find_chunk(Chunk *curr, void *ptr);
Chunk *find_available_chunk(Chunk *curr, size_t size);
Chunk *carve_chunk(Chunk *available_chunk, size_t size);
Chunk *fragment_chunk(Chunk* curr, size_t data_size);

#endif
