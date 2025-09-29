#include <unistd.h> 
#include <stddef.h> 
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "chunk.h"

static Chunk *global_head_ptr = NULL;

// Round up the requested size (called from the user in the malloc, calloc, or
// realloc) to the nearest ALLIGN bytes.
// @param size User requested size in bytes.
// @return Bytes requested rounded to the nearest multiple of ALLIGN.
size_t block_size(size_t size) {
  size_t mod = size % ALLIGN;
  if (mod != 0) {
    size = size + (ALLIGN - mod);
  }
  return size;
}

// Get the head (first Chunk) of the linked list.
// If this is the first time you are using one of the four functions, initalize
// the datastructure to the defaults, and then return the newly created Chunk.
// @return A Chunk* to the first element in the linked list.
Chunk *get_head() {
  if (global_head_ptr == NULL) {
    // Make sure that the program break starts at an even multiple of ALLIGN.
    // Every allocation after this point should be in a multiple of ALLIGN 
    // as well to maintain pointer consistency.
    void *floor = sbrk(0);
    if (floor == (void *)-1 || sbrk((uintptr_t)floor % ALLIGN) == (void *)-1) {
      return NULL;
    }
  
    // Move the break to the inital "heap" size.
    global_head_ptr = sbrk(HUNK_SIZE);
    if (global_head_ptr == (void *)-1) {
      return NULL;
    }

    // The usable space in any chunk does not include the size of the header
    // (or Chunk struct)
    global_head_ptr->size = HUNK_SIZE - CHUNK_SIZE;
    global_head_ptr->is_available = true;
    global_head_ptr->prev= NULL;
    global_head_ptr->next = NULL;
  }
  return global_head_ptr;
}

// Merges two chunks together into one element of the linked list, updating the
// next and prev pointers accordingly. curr->next gets absorbed into curr's 
// data section if curr is not the last element in the linked list.
// @param curr A Chunk* of the target Chunk.
// @return A Chunk* to the newly merged Chunk (curr).
Chunk *merge_next(Chunk *curr) {
  // If we are at the tail, there is no "next" chunk to merge, and we can 
  // break early, returning curr's Chunk*.
  if (curr->next != NULL) {
    // The new size must also include the size of the Header of the next chunk.
    // The next chunk will be "skipped" making the data in that header
    // useless.
    size_t new_size = curr->size + CHUNK_SIZE + curr->next->size;
    curr->size = new_size;
  
    // Curr's next should now point to the next->next Chunk, effectively 
    // "skipping" the next Chunk
    Chunk *temp = curr->next->next;
    curr->next = temp;

    // If the next chunk is the tail of the linked list, then we can't update
    // it's previous pointer (it will be null, not a Chunk struct).
    // However, if it does exist, update the previous pointer.
    if (temp != NULL) {
      temp->prev = curr;
    }
  }
  return curr;
}

// Merges two chunks together into one element of the linked list, updating the
// next and prev pointers accordingly. 
// curr gets absorbed into curr->next's data section if curr is not the first
// element in the linked list.
// @param curr A Chunk* of the target Chunk.
// @return A Chunk* to the newly merged Chunk (curr->prev or curr).
Chunk *merge_prev(Chunk *curr) {
  // If we are at the tail, there is no "next" chunk to merge, and we can 
  // break early, returning curr's Chunk*.
  if (curr->prev != NULL) {
    // The new size must also include the size of the Header of the curr chunk.
    // The current chunk will be "skipped" making the data in that header
    // useless.
    size_t new_size = curr->prev->size + CHUNK_SIZE + curr->size;
    curr->prev->size = new_size;

    // The previous pointer should point to the curr->next, essentially
    // "skipping" the curr node.
    curr->prev->next = curr->next;
    
    // If the next chunk is the tail of the linked list, then we can't update
    // it's previous pointer (it will be null, not a Chunk struct).
    // However, if it does exist, update the previous pointer.
    if (curr->next != NULL) {
      curr->next->prev = curr->prev;
    }

    return curr->prev;
  }
  // There is no previous chunk to return, so just return the current without 
  // doing anything special.
  return curr;
}


// Gets the Chunk that is associated with the data pointer (the pointer that 
// the user will be referencing in their programs through a linear recursive
// search.
// @param curr A Chunk* whose data section is to be checked with the given 
// pointer.
// @param ptr The given pointer which the user provides.
// @return A Chunk* to the associated data ptr. NULL if no suitable poiner is 
// found.
Chunk *find_chunk(Chunk *curr, void *ptr) {
  // The address of the Chunk plus the size that the Chunk header takes up will
  // give us an address to the data section of the chunk we are looking at.
  void *curr_data_start = (void *)((uintptr_t)curr + CHUNK_SIZE);
  void *curr_data_stop = (void *)((uintptr_t)curr_data_start + curr->size);

  // We have found a valid chunk if the addresses fall under the range of the
  // data segment.
  if (curr_data_start <= ptr && ptr <= curr_data_stop) {
    return curr;
  }

  // If the tail is reached and the curr_data address has not matched at 
  // this point, then return NULL to indicate so. It is not our problem.
  if (curr->next == NULL) {
    return NULL;
  }

  // Recursively linear search through our linked list.
  return find_chunk(curr->next, ptr);
}

// Finds the earliest available Chunk through a linear recursive search.
// This function looks for Chunks that are not being used. Another requirement
// is having enough space in the Chunks data section to allow for the size 
// requested, and a "minimum" chunk (just the size of the Chunk header and the
// ALLIGN size).
// @param curr The current Chunk* which will be checked for the requirements
// mentioned above.
// @param size The size of the space we are looking for.
// @return A Chunk* to the Chunk which meets the requirements mentioned above.
Chunk *find_available_chunk(Chunk *curr, size_t size) {
  // We have found a valid match.
  if (curr->is_available && curr->size >= size + CHUNK_SIZE + ALLIGN) {
    return curr;
  }
  
  // If the tail is reached and the curr_data address has not matched at 
  // this point, then there is no suitable space.
  if (curr->next == NULL){
    // Increase the hunk to make more space.
    if (sbrk(HUNK_SIZE) == (void *)-1) {
      return NULL;
    }

    if (curr->is_available) {
      // If the tail is not being used, then tack the HUNK_SIZE to the end of
      // it without creating a new Chunk.
      curr->size = curr->size + HUNK_SIZE;
    }
    // else {
    //   return NULL;
    // }

    // Call the find_available_chunk on the tail again.
    // If there is enough space, then it will be allocated. If not, then this
    // branch will be executed till there is enough space in the Hunk.
    return find_available_chunk(curr, size);
  }
  
  // Recursively linear search through our linked list.
  return find_available_chunk(curr->next, size);
}

// Splits a chunk that has enough space into two portions, creating a Chunk
// in the process. The newly created chunk will be set to available.
// @param curr A Chunk that is going to be split.
// @param size The size of the new chunk. 
// @return A Chunk* to the chunk whose size was split to the spesification. 
Chunk *carve_chunk(Chunk *curr, size_t size) {
  // Create a remainder_chunk at address offset size bytes away, allocating 
  // the remaining bytes as it's size. Copy the availability from the current.
  // Update the prev and next pointers.
  Chunk *remainder_chunk = (Chunk*)((uintptr_t)curr + CHUNK_SIZE + size);
  remainder_chunk->size = curr->size - size - CHUNK_SIZE;
  remainder_chunk->is_available = true; 
  remainder_chunk->prev = curr;
  remainder_chunk->next = curr->next;

  // If the current Chunk is not the tail, then it has a "next" chunk. In that
  // case, update the "next"s prev pointer.
  if (curr->next != NULL) {
    curr->next->prev = remainder_chunk;
  }
 
  // Update the current chunk to have the desired size. Update the next 
  // pointers accordingly. Note: the prev pointer will stay the same.
  curr->size = size;
  // curr->prev = curr->prev; // stays the same
  curr->next = remainder_chunk;

  // The curr now has the new size! Return our beautiful work.
  return curr;
}



