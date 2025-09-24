#include <stddef.h> 
#include <stdio.h> 
#include <stdbool.h> 
#include <unistd.h> 

#include "unk.h"

void *calloc(size_t nmemb, size_t size) {
  Chunk *head = get_head();
  if (head == NULL) {
    // TODO: should libraries give perrors?
    // or should they just return their values that indicate an error?
    perror("malloc: error getting head ptr");
    return NULL;
  }
  return NULL;
}

void *my_malloc(size_t size) {
  // initalize the first chunk in the hunk (the head of the linked list)
  Chunk *head = get_head();
  if (head == NULL) {
    // TODO: should libraries give perrors?
    // or should they just return their values that indicate an error?
    perror("malloc: error getting head ptr");
    return NULL;
  }

  // make sure that the size that the user alllocates is in multiples of 16
  size = block_size(size);

  Chunk *available_chunk = find_available_chunk(head, size);
  if (available_chunk == NULL) {
    perror("malloc: error finding available chunk");
    return NULL;
  }

  if (carve_chunk(available_chunk, size, false) == NULL) {
    perror("malloc: error carving available chunk");
    return NULL;
  }
  
  return available_chunk;
}

void free(void *ptr) {
  // initalize the first chunk in the hunk (the head of the linked list)
  Chunk *head = get_head();
  if (head == NULL) {
    // TODO: should libraries give perrors?
    // or should they just return their values that indicate an error?
    perror("malloc: error getting head ptr");
    return NULL;
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
