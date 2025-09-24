#ifndef UNK
#define UNK

#include <stddef.h>
#include "datastructures.h"

void *merge_next(Chunk *curr);
void *merge_prev(Chunk *curr);
Chunk *get_head();
Chunk *find_available_chunk(Chunk *curr, size_t size);
Chunk *carve_chunk(Chunk *available_chunk, size_t size, bool initalize);
size_t block_size(size_t size);

#endif
