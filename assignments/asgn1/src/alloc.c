#include <stddef.h> 
#include <stdio.h> 
#include <stdbool.h> 
#include <stdint.h> 
#include <unistd.h> 

#include "unk.h"

void *calloc(size_t nmemb, size_t size) {
  // get the first chunk of the linked list
  // if this is the first time using it, initalize the list with a global var
  Chunk *head = get_head();
  if (head == NULL) {
    perror("calloc: error getting head ptr");
    return NULL;
  }

  // round the user's alloc request to the nearest multiple of 16 
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

void free(void *ptr) {
  // initalize the first chunk in the hunk (the head of the linked list)
  Chunk *head = get_head();
  if (head == NULL) {
    // TODO: should libraries give perrors?
    // or should they just return their values that indicate an error?
    perror("malloc: error getting head ptr");
  }
}

void *realloc(void *ptr, size_t size) {
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
