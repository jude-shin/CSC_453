#include <stddef.h> 
#include <stdio.h> 
#include <stdbool.h> 
#include <stdint.h> 
#include <unistd.h> 
#include <string.h> 

#include "alloc.h"
#include "chunk.h"

void *my_calloc(size_t nmemb, size_t size) {
  // TODO: is this expected behavior?
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
  // multiple of ALLIGN 
  size = block_size(size);

  // find the next available chunk, increasing the hunk size as needed
  Chunk *available_chunk = find_available_chunk(head, size);
  if (available_chunk == NULL) {
    perror("calloc: error finding available chunk");
    return NULL;
  }

  // split the available data portion into a new chunk, and mark the allocated
  // chunk as being used
  carve_chunk(available_chunk, size, true);
 
  // return the pointer that is useful to the user (not the chunk pointer)
  return (void*)((uintptr_t)available_chunk + CHUNK_SIZE);
}

void *my_malloc(size_t size) {
  // Edge Cases
  // TODO: is this an expected requirement?
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

  // round the user's alloc request to the nearest multiple of ALLIGN
  size = block_size(size);

  // find the next available chunk, increasing the hunk size as needed
  Chunk *available_chunk = find_available_chunk(head, size);
  if (available_chunk == NULL) {
    perror("malloc: error finding available chunk");
    return NULL;
  }

  // split the available data portion into a new chunk, and mark the allocated
  // chunk as being used
  carve_chunk(available_chunk, size, false);
 
  // return the pointer that is useful to the user (not the chunk pointer)
  return (void*)((uintptr_t)available_chunk + CHUNK_SIZE);
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
  Chunk *freeable_chunk = find_chunk(head, ptr);
  if (freeable_chunk == NULL) {
    perror("free: pointer is not valid");
  }
  if (freeable_chunk->is_available) {
    perror("free: chunk already available");
  }

  // then have that function return the chunk * that is to be freed
  freeable_chunk->is_available = true;

  // then you do the stuff with the free and the merge
  Chunk *merged_chunk = freeable_chunk; // var renaming for bookeeping
  if (merged_chunk->next != NULL && merged_chunk->next->is_available) {
    merged_chunk = merge_next(merged_chunk);
  }
  if (merged_chunk->prev != NULL && merged_chunk->prev->is_available) {
    merged_chunk = merge_prev(merged_chunk);
  }
}

void *my_realloc(void *ptr, size_t size) {
  // Edge Cases
  if (ptr == NULL) {
    return my_malloc(size);
  }
  else if (size == 0) {
    my_free(ptr);
    // TODO what to return?
    return NULL; // NOTE footer(3) says "these" are equivalent
  }

  // get the first chunk of the linked list
  // if this is the first time using it, initalize the list with a global var
  Chunk *head = get_head();
  if (head == NULL) {
    perror("realloc: error getting head ptr");
    return NULL;
  }

  // find the chunk that lines up with the ptr requested
  Chunk *curr = find_chunk(head, ptr);
  if (curr == NULL) {
    perror("realloc: pointer is not valid");
    return NULL;
  }
  
  // try to merge in place to prevent copying a ton of data
  curr->is_available = true;

  // get the new size of the chunk-to-be
  size_t data_size = size + curr->size;

  // round the user's alloc request to the nearest multiple of ALLIGN 
  data_size = block_size(data_size);

  if (curr->next != NULL && 
      curr->next->is_available &&
      data_size < curr->size + CHUNK_SIZE + curr->next->size + CHUNK_SIZE) {
    curr->is_available = true;
    curr = merge_next(curr);

    carve_chunk(curr, size, true);

    return (void*)((uintptr_t)curr + CHUNK_SIZE);
  }
  
  if (curr->prev != NULL && curr->prev->is_available) {
    curr = merge_prev(curr);
  }

  // whatever merges happen, we don't care at this point.
  // we should just try to look for a new chunk with the size of the original,
  // plus the extra that the user requested in the realloc
  void *dst_data = my_malloc(data_size);
  
  memmove(dst_data, ptr, data_size);

  return dst_data;
}

