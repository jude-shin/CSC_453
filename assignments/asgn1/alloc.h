#ifndef ALLOC
#define ALLOC

#include <stddef.h>

// Allocates memory of nmemb*size bytes. Sets everything to 0.
void *calloc(size_t nmemb, size_t size);
// Allocates memory of size bytes. Contents are not guarenteed.
void *malloc(size_t size);
// De-Allocates previously allocated memory to be used again at ptr.
void free(void *ptr);
// Change the size of a previously allocated chunk of memory at ptr to 
// size bytes.
void *realloc(void *ptr, size_t size);

#endif
