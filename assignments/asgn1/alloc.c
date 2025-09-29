#include <stddef.h> 
#include <stdio.h> 
#include <stdbool.h> 
#include <stdlib.h> 
#include <stdint.h> 
#include <string.h> 
#include <pp.h>

#include "alloc.h"
#include "chunk.h"

// Allocates a chunk of memory, setting all of the data inside to 0. Gives a
// convinient way to allocate memory for an array.
// @param nmemb Number of elements to be allocated.
// @param size Size of each element to be allocated.
// @return A void* to the usable data portion.
void *calloc(size_t nmemb, size_t size) {
  // Edge Cases
  // TODO: is this expected behavior? 
  if (nmemb == 0 || size == 0) {
    return NULL;
  }

  // Get the first chunk of the linked list. This is stored as a global var.
  // If this is the first time using it, initalize the list with the defaults.
  Chunk *head = get_head();
  if (head == NULL) {
    perror("calloc: error getting head ptr");
    return NULL;
  }

  // Simulate an array by giving space for nmemb elements of size size.
  size_t data_size = nmemb*size;

  // Round the data size to the nearest multiple of ALLIGN
  data_size = block_size(data_size);

  // Find the next available chunk, increasing the hunk size as needed.
  Chunk *available_chunk = find_available_chunk(head, data_size);
  if (available_chunk == NULL) {
    perror("calloc: error finding available chunk");
    return NULL;
  }

  // Split the available data portion into a new chunk.
  Chunk *new_chunk = carve_chunk(available_chunk, size);

  // Mark the first chunk as being allocated (Chunk who got the desired size).
  new_chunk->is_available = false;

  // Set all the data to be zeros.
  void *data = (void*)((uintptr_t)new_chunk + CHUNK_SIZE);
  memset(data, 0, data_size);

  // Debugging output if env var is present.
  if (getenv("DEBUG_MALLOC") != NULL){

    char buffer[171];

    snprintf(
        buffer, 
        sizeof(buffer),
        // "MALLOC: calloc(%020d, %020d) => (ptr=0x%018lx, size=%020d)\n", 
        "MALLOC: calloc(%020d, %020d) => (prev=0x%018lx, ptr=0x%018lx, next=0x%018lx, size=%020d)\n", 
        (int)nmemb,
        (int)size, 
        (uintptr_t)available_chunk->prev,
        (uintptr_t)available_chunk + CHUNK_SIZE,
        (uintptr_t)available_chunk->next,
        (int)new_chunk->size);

    fputs(buffer, stdout);
  }

  // Return the pointer that is useful to the user (not the chunk pointer).
  return (void*)((uintptr_t)available_chunk + CHUNK_SIZE);
}

// Allocates a chunk of memory, data inside is not guarenteed.
// @param size Size of bytes to be allocated.
// @return A void* to the usable data portion.
void *malloc(size_t size) {
  // Edge Cases
  // Edge Cases
  // TODO: is this an expected requirement?
  if (size == 0) {
    return NULL;
  }

  // Get the first chunk of the linked list. This is stored as a global var.
  // If this is the first time using it, initalize the list with the defaults.
  Chunk *head = get_head();
  if (head == NULL) {
    perror("malloc: error getting head ptr");
    return NULL;
  }

  // Round the size request to the nearest multiple of ALLIGN.
  size_t data_size = block_size(size);

  // Find the next available chunk, increasing the hunk size as needed.
  Chunk *available_chunk = find_available_chunk(head, data_size);
  if (available_chunk == NULL) {
    perror("malloc: error finding available chunk");
    return NULL;
  }

  // Split the available data portion into a new chunk.
  Chunk *new_chunk = carve_chunk(available_chunk, data_size);
  if (new_chunk == NULL) {
    perror("malloc: error carving chunk");
    return NULL;
  }

  // Mark the first chunk as being allocated (Chunk who got the desired size).
  new_chunk->is_available = false;

  // Debugging output if env var is present.
  if (getenv("DEBUG_MALLOC") != NULL){
    char buffer[150];

    snprintf(
        buffer, 
        sizeof(buffer),
        // "MALLOC: malloc(%020d) => (ptr=0x%018lx, size=%020d)\n", 
        "MALLOC: malloc(%020d) => \n(prev=0x%018lx, ptr=0x%018lx, next=0x%018lx, size=%020d)\n", 
        (int)size, 
        (uintptr_t)available_chunk->prev,
        (uintptr_t)available_chunk + CHUNK_SIZE,
        (uintptr_t)available_chunk->next,
        (int)new_chunk->size);

    fputs(buffer, stdout);
  }
 
  // Return the pointer that is useful to the user (not the chunk pointer).
  return (void*)((uintptr_t)new_chunk + CHUNK_SIZE);
}

// De-Allocates the chunk of memory given my malloc, calloc, or realloc.
// Allows for the chunks of memory to be used elsewhere.
// @param ptr The pointer to the previously alloced portion of memory.
// @return void.
void free(void *ptr) {
  // Edge Cases
  if (ptr == NULL){
    return;
  }

  // Get the first chunk of the linked list. This is stored as a global var.
  // If this is the first time using it, initalize the list with the defaults.
  Chunk *head = get_head();
  if (head == NULL) {
    perror("free: error getting head ptr");
    return;
  }
  
  // NOTE: if the user supposedly wanted to free the "head", but did not end up 
  // allocating any memory first, the head will be initalized. However, it will
  // be initalized to "available". Trying to free an available chunk will 
  // result in a different error.
  // TODO: maybe you should put the catch at the front to make it unambiguous
  
  // Start from the head and linear search through all of the chunks, seeing if
  // any of the addresses line up. 
  Chunk *freeable_chunk = find_chunk(head, ptr);
  if (freeable_chunk == NULL) {
    perror("free: pointer is not valid");
    return;
  }
  // Only accept the chunk if it is allocated (if it is being used)
  if (freeable_chunk->is_available) {
    perror("free: chunk already available");
    return;
  }
  
  freeable_chunk->is_available = true;

  // Debugging output if env var is present.
  if (getenv("DEBUG_MALLOC") != NULL){
    char buffer[118];

    snprintf(
        buffer, 
        sizeof(buffer),
        // "MALLOC: malloc(%020d) => (ptr=0x%018lx, size=%020d)\n", 
        "MALLOC: free(0x%018lx)\n", 
        (uintptr_t)ptr);

    fputs(buffer, stdout);
  }

  // Check to see if you can merge adjacent chunks that might also be 
  // available.
  Chunk *merged_chunk = freeable_chunk;
  // Merge the freeable_chunk->next Chunk into the freeable_chunk.
  // Keeping the current freeable_chunk.
  if (merged_chunk->next != NULL && merged_chunk->next->is_available) {
    merged_chunk = merge_next(merged_chunk);
  }
  // Merge the freeable_chunk Chunk into the freeable_chunk->prev.
  // Keeping the previous chunk if available, otherwise, we keep the 
  // freeable_chunk.
  if (merged_chunk->prev != NULL && merged_chunk->prev->is_available) {
    merged_chunk = merge_prev(merged_chunk);
  }
}

// Increases the size of a previously alloced portion of memory. Data inside
// is not guarenteed, and in-place copying is favored.
// @param ptr The pointer to the previously alloced portion of memory.
// @param size Size of bytes to be added onto the previously allocated chunk
// of memory.
// @return A void* to the usable data portion.
void *realloc(void *ptr, size_t size) {
  // Edge Cases
  if (ptr == NULL) {
    return malloc(size);
  }
  else if (size == 0) {
    free(ptr);
    // TODO what to return?
    return NULL; // NOTE footer(3) says "these" are equivalent
  }

  // Get the first chunk of the linked list. This is stored as a global var.
  // If this is the first time using it, initalize the list with the defaults.
  Chunk *head = get_head();
  if (head == NULL) {
    perror("realloc: error getting head ptr");
    return NULL;
  }

  // Start from the head and linear search through all of the chunks, seeing if
  // any of the addresses line up. 
  Chunk *curr = find_chunk(head, ptr);
  if (curr == NULL) {
    perror("realloc: pointer is not valid");
    return NULL;
  }

  // The new size of the chunk-to-be.
  size_t data_size = size + curr->size;

  // Round the size request to the nearest multiple of ALLIGN.
  data_size = block_size(data_size);

  // Try to merge in place to prevent copying a ton of data if the Chunk next 
  // to the chunk is able to hold another chunk.
  if (curr->next != NULL && 
      curr->next->is_available &&
      curr->size >= data_size + CHUNK_SIZE + ALLIGN) { // TODO: test this

    // Make the chunk bigger by merging the next Chunk into the curr Chunk's
    // data section.
    curr = merge_next(curr);
    
    // Size was checked beforehand, so this will never error. 
    Chunk *new_chunk = carve_chunk(curr, data_size);

    new_chunk->is_available = true;

    // Debugging output if env var is present.
    if (getenv("DEBUG_MALLOC") != NULL){
    char buffer[173];

      snprintf(
          buffer, 
          sizeof(buffer),
          // "MALLOC: realloc(0x%018lx, %020d) => (ptr=0x%018lx, size=%020d)\n", 
          "MALLOC: realloc(0x%018lx, %020d) => \n(prev=0x%018lx, ptr=0x%018lx, next=0x%018lx, size=%020d)\n", 
          (uintptr_t)ptr,
          (int)size, 
          (uintptr_t)new_chunk->prev,
          (uintptr_t)new_chunk + CHUNK_SIZE,
          (uintptr_t)new_chunk->next,
          (int)data_size);

      fputs(buffer, stdout);
    }

    return (void*)((uintptr_t)new_chunk + CHUNK_SIZE); //TODO check this
    // return (void*)((uintptr_t)curr + CHUNK_SIZE);
  }
  
  // If copy in place did not work out, then free the current chunk, giving
  // a chance for the adjacent chunks to merge.
  free((void*)((uintptr_t)curr + CHUNK_SIZE));
  
  // Find a new home for the data with malloc.
  void *dst_data = malloc(data_size);
 
  // Copy the data over to the new location, no matter where the new
  // data was allocated.
  memmove(dst_data, ptr, data_size);

  // Debugging output if env var is present.
  if (getenv("DEBUG_MALLOC") != NULL){
    char buffer[118];

    snprintf(
        buffer, 
        sizeof(buffer),
        "MALLOC: realloc(0x%018lx, %020d) => (ptr=0x%018lx, size=%020d)\n", 
        (uintptr_t)ptr,
        (int)size, 
        (uintptr_t)dst_data,
        (int)data_size);

    fputs(buffer, stdout);
  }

  return dst_data;
}

