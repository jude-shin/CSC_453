#include <stddef.h> 
#include <stdio.h> 
#include <stdbool.h> 
#include <stdlib.h> 
#include <stdint.h> 
#include <string.h> 

#include <unistd.h>

#include "alloc.h"
#include "chunk.h"

// Allocates a chunk of memory, setting all of the data inside to 0. Gives a
// convinient way to allocate memory for an array.
// @param nmemb Number of elements to be allocated.
// @param size Size of each element to be allocated.
// @return A void* to the usable data portion.
void *calloc(size_t nmemb, size_t size) {
  // Edge Cases
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
  Chunk *new_chunk = carve_chunk(available_chunk, data_size);

  // Mark the first chunk as being allocated (Chunk who got the desired size).
  new_chunk->is_available = false;

  // Set all the data to be zeros.
  void *data = (void*)((uintptr_t)new_chunk + CHUNK_SIZE);
  memset(data, 0, data_size);

  // Debugging output if env var is present.
  if (getenv("DEBUG_MALLOC") != NULL){
    char buffer[117] = {0};

    snprintf(
        buffer, 
        sizeof(buffer),
        "MALLOC: calloc(%d, %d) => (ptr=%p, size=%d)\n", 
        (int)nmemb,
        (int)size, 
        (void*)((uintptr_t)available_chunk + CHUNK_SIZE),
        (int)new_chunk->size);

    write(STDOUT_FILENO, buffer, strlen(buffer));
  }

  // Return the pointer that is useful to the user (not the chunk pointer).
  return (void*)((uintptr_t)available_chunk + CHUNK_SIZE);
}

// Allocates a chunk of memory, data inside is not guarenteed.
// @param size Size of bytes to be allocated.
// @return A void* to the usable data portion.
void *malloc(size_t size) {
  // Edge Cases
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

  // Mark the first chunk as being allocated (Chunk who got the desired size).
  new_chunk->is_available = false;

  // Debugging output if env var is present.
  if (getenv("DEBUG_MALLOC") != NULL){
    char buffer[95] = {0};

    snprintf(
        buffer, 
        sizeof(buffer),
        "MALLOC: malloc(%d) => (ptr=%p, size=%d)\n", 
        (int)size, 
        (void*)((uintptr_t)available_chunk + CHUNK_SIZE),
        (int)new_chunk->size);

    write(STDOUT_FILENO, buffer, strlen(buffer));
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

  // Start from the head and linear search through all of the chunks, seeing if
  // any of the addresses line up. 
  Chunk *freeable_chunk = find_chunk(head, ptr);
  // No chunk was found, and this was an error on the users part. Not our prob.
  if (freeable_chunk == NULL) {
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
    char buffer[36] = {0};

    snprintf(
        buffer, 
        sizeof(buffer),
        "MALLOC: free(%p)\n", 
        ptr);

    write(STDOUT_FILENO, buffer, strlen(buffer));
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
    return NULL;
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

  // No chunk was found, and this was an error on the users part. Not our prob.
  if (curr == NULL) {
    return NULL;
  }

  // Round the size request to the nearest multiple of ALLIGN.
  size_t data_size = block_size(size);
  Chunk *new_chunk = NULL; 

  // Try to merge in place to prevent copying a ton of data if the Chunk next 
  // to the chunk is able to hold another chunk.
  if (data_size <= curr->size) {
    // COPY IN PLACE (make chunk smaller)

    // Try to fragment the data.
    new_chunk = fragment_chunk(curr, data_size);
  }
  else if (curr->next != NULL && 
    curr->next->is_available &&
    data_size <= curr->size + CHUNK_SIZE + curr->next->size) {
    // COPY IN PLACE (make chunk larger)

    // Make the chunk bigger by merging the next Chunk into the curr Chunk's
    // data section.
    curr = merge_next(curr);

    // Try to fragment the data.
    new_chunk = fragment_chunk(curr, data_size);
  }
  else {
    // If copy in place did not work out, then free the current chunk, giving
    // a chance for the adjacent chunks to merge.
    free((void*)((uintptr_t)curr + CHUNK_SIZE));

    // Find a new home for the data with malloc.
    void *dst_data = malloc(data_size);

    // Get the header for this piece of data.
    new_chunk = (void *)((uintptr_t)dst_data - CHUNK_SIZE);

    // Copy the data over to the new location, no matter where the new
    // data was allocated.
    memmove(dst_data, ptr, size);
  }

  // Debugging output if env var is present.
  if (getenv("DEBUG_MALLOC") != NULL){
    char buffer[118] = {0};

    snprintf(
        buffer, 
        sizeof(buffer),
        "MALLOC: realloc(%p, %d) => (ptr=%p, size=%d)\n", 
        ptr,
        (int)size, 
        (void*)((uintptr_t)new_chunk + CHUNK_SIZE),
        (int)data_size);

    write(STDOUT_FILENO, buffer, strlen(buffer));
  }

  return (void*)((uintptr_t)new_chunk + CHUNK_SIZE);
}
