#include <stddef.h> 
#include <stdio.h> 

#include "chunk.h"


#define CHUNK_SIZE 16

void *my_calloc(size_t nmemb, size_t size) {
  printf("hello from calloc");
  return NULL;
}

void *my_malloc(size_t size) {
  printf("hello from malloc");
  return NULL;
}

void my_free(void *ptr) {
  printf("hello from free");
}

void *my_realloc(void *ptr, size_t size) {
  printf("hello from realloc");
  return NULL;
}
