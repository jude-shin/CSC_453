#include <stddef.h> 
#include <stdio.h> 
#include <stdbool.h> 
#include <stdint.h> 
#include <unistd.h> 

#include "unk.h"

void *my_calloc(size_t nmemb, size_t size) {
  // check if this is expected behavior
  if (nmemb == 0 || size == 0) {
    return NULL;
  }
  // get the first chunk of the linked list
  // if this is the first time using it, initalize the list with a global var
  Chunk *head = get_head();
  if (head == NULL) {
    perror("calloc: error getting head ptr");
    return NULL;
  }

  // simulate an array by giving space for nmemb elements of size size
  size = nmemb*size;

  // round the remainder of the total space needed up to the nearest
  // multiple of 16
  size = block_size(size);

  // find the next available chunk, increasing the hunk size as needed
  Chunk *available_chunk = find_available_chunk(head, size);
  if (available_chunk == NULL) {
    perror("calloc: error finding available chunk");
    return NULL;
  }

  // split the available data portion into a new chunk, and mark the allocated
  // chunk as being used
  if (carve_chunk(available_chunk, size, true) == NULL) {
    perror("calloc: error carving available chunk");
    return NULL;
  }
 
  // return the pointer that is useful to the user (not the chunk pointer)
  return (void*)((uintptr_t)available_chunk + sizeof(Chunk));
}

void *my_malloc(size_t size) {
  // check edge cases
  if (size == 0) {
    return NULL;
  }

  // get the first chunk of the linked list
  // if this is the first time using it, initalize the list with a global var
  Chunk *head = get_head();
  if (head == NULL) {
    perror("malloc: error getting head ptr");
    return NULL;
  }

  // round the user's alloc request to the nearest multiple of 16 
  size = block_size(size);

  // find the next available chunk, increasing the hunk size as needed
  Chunk *available_chunk = find_available_chunk(head, size);
  if (available_chunk == NULL) {
    perror("malloc: error finding available chunk");
    return NULL;
  }

  // split the available data portion into a new chunk, and mark the allocated
  // chunk as being used
  if (carve_chunk(available_chunk, size, false) == NULL) {
    perror("malloc: error carving available chunk");
    return NULL;
  }
 
  // return the pointer that is useful to the user (not the chunk pointer)
  return (void*)((uintptr_t)available_chunk + sizeof(Chunk));
}

void my_free(void *ptr) {
  // get the first chunk of the linked list
  // if this is the first time using it, initalize the list with a global var
  Chunk *head = get_head();
  if (head == NULL) {
    perror("free: error getting head ptr");
  }
  
  // NOTE: if the user supposedly wanted to free the "head", but did not end up 
  // allocating any memory first, the head will be initalized. However, it will
  // be initalized to "available". Trying to free an available chunk will 
  // result in a different error.
  // TODO: maybe you should put the catch at the front to make it unambiguous
  
  // start from the head
  // linear search through all of the chunks, seeing if any of the addresses 
  // line up. at the same time, check to see if the chunk is available or not.
  // if it lines up, and it is currently unavailable (was allocated),
  Chunk *freeable_chunk = find_freeable_chunk(head, ptr);
  if (freeable_chunk == NULL) {
    perror("free: no valid chunk to be freed");
  }

  // then have that function return the chunk * that is to be freed
  freeable_chunk->is_available = true;

  // then you do the stuff with the free and the merge
  Chunk *merged_chunk = merge_next(freeable_chunk);
  merged_chunk = merge_prev(merged_chunk); // you can't really use this
}

void *my_realloc(void *ptr, size_t size) {
  // initalize the first chunk in the hunk (the head of the linked list)
  Chunk *head = get_head();
  if (head == NULL) {
    // TODO: should libraries give perrors?
    // or should they just return their values that indicate an error?
    perror("malloc: error getting head ptr");
    return NULL;
  }
  return NULL;
}
