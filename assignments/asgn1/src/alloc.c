#include <stddef.h> 
#include <stdio.h> 
#include <unistd.h> 

#include "unk.h"

void *calloc(size_t nmemb, size_t size) {
  return NULL;
}

void *malloc(size_t size) {
  // initalize the first chunk in the hunk (the head of the linked list)
  void *hello = get_head();

  // check to see if we have size in the hunk (only if trying to allocate to
  // the tail)
  // this gets the "to be" address. it adds the following
  // size of the tail header, size of the data of tail, size of the to be 
  // header, and the size of the to be data
  // size_t theoretical = sizeof(Chunk) + [get the data size from tail header] 
  // + sizeof(Chunk) + size





  // every time that we calll 
  return NULL;
}

void free(void *ptr) {
}

void *realloc(void *ptr, size_t size) {
  return NULL;
}
