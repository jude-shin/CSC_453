#include <stddef.h> 
#include <stdio.h> 
#include <stdbool.h> 
#include <unistd.h> 

#include "unk.h"

void *calloc(size_t nmemb, size_t size) {
  // void *head_addr = get_head_addr();
  return NULL;
}

void *my_malloc(size_t size) {
  // TODO: if anything returns NULL, then you should exit with a werid status.
  // Check after every function call
  // initalize the first chunk in the hunk (the head of the linked list)
  Chunk *head = get_head();
  if (head == NULL) {
    // TODO: should libraries give perrors?
    // or should they just return their values that indicate an error?
    perror("malloc: error getting head ptr");
    return NULL;
  }

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
}

void *realloc(void *ptr, size_t size) {
  return NULL;
}
