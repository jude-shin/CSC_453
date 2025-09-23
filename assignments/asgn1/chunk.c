#include "chunk.h"

/*
Checks the current and next chunks, to clean up space.
If curr chunk is not being used, and next chunk is also not being used, 
keep the curr header, and turn the rest of space into the data of the curr
header. 
Repeat recursively (setting the prev header as curr in this function) 
until the end is reached, or there is a chunk that is being used.
*/
int merge_next(Chunk *curr) {
  return 0;
}

/*
Checks the current and prev chunks, to clean up space.
If curr chunk is not being used, and prev chunk is also not being used, 
keep the prev header, and turn the rest of space into the data of the prev 
header. 
Repeat recursively (setting the prev header as curr in this function)
until the end is reached, or there is a chunk that is being used.
*/
int merge_prev(Chunk *curr) {
  return 0;
}
