#include <unistd.h> 
#include <stddef.h> 
#include <stdbool.h>  // do I even need this?
#include <string.h>
#include <stdint.h>

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
    uintptr_t floor = (uintptr_t)sbrk(0);
    if (sbrk(floor % ALLIGN) == (void *)-1) {
      return NULL;
    }
  
    // Move the break to the inital "heap" size.
    global_head_ptr = sbrk(HUNK_SIZE);
    if (global_head_ptr == NULL) {
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
  // The address of the data section of the given Chunk we are checking.
  // The address of the Chunk plus the size that the Chunk header takes up.
  void *curr_data = (void *)((uintptr_t)curr + CHUNK_SIZE);

  // We have found a valid chunk if the addresses match.
  if (curr_data == ptr) {
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

Chunk *find_available_chunk(Chunk *curr, size_t size) {
  // start from the head and recursively search
  // this inequality allows us to guarentee that the space we will allocate
  // will have enough for the header, the data space, and the new header space
  // at the end
  if (curr->is_available && curr->size > size + CHUNK_SIZE) {
    // then we are able to allocate it!
    // this chunk is the chosen one
    return curr;
  }

  if (curr->next == NULL){
    // if we have reached here, then there is no suitable space
    // in this case, we must increase the hunk with sbrk()
    sbrk(HUNK_SIZE);

    if (curr->is_available) {
      // then we want to add another HUNK_SIZE space to the tail
      curr->size = curr->size + HUNK_SIZE;

      // then call the find_available_chunk on the tail again.
      // if there is space the second time around, horray!
      // if not, that means that the requested malloc is greater than the
      // HUNK_SIZE, so we will just do it again. We CANNOT assume that the
      // requested malloc size will be less than the HUNK_SIZE
    }
    // NOTE: the tail should always be available
    return find_available_chunk(curr, size);
  }
  
  // recursively linear search through our linked list
  return find_available_chunk(curr->next, size);
}

Chunk *carve_chunk(Chunk *available_chunk, size_t size, bool initalize) {
  // at this point we have a chunk that can be used for the data that we want

  // 2) create a new_chunk at address (available_chunk + CHUNK_SIZE + size)
  // Chunk *new_chunk = (available_chunk + 1) + size;
  Chunk *new_chunk = (Chunk*)((uintptr_t)available_chunk + CHUNK_SIZE + size);

  // 3) populate that new header with the correct information 
  new_chunk->size = available_chunk->size - size - CHUNK_SIZE;
  new_chunk->is_available = true; // takes remaining available size
  new_chunk->prev = available_chunk;
  new_chunk->next = available_chunk->next;

  // makes sure that when the available_chunk is NOT the last element,
  // the prev pointer will point to the newly carved chunk, not the previous
  // available_chunk
  if (available_chunk->next != NULL) {
    available_chunk->next->prev = new_chunk;
  }
 
  if (initalize) {
    void *data = (void*)((uintptr_t)available_chunk + CHUNK_SIZE);
    memset(data, 0, size);
  }

  // update the available_chunk to have the correct information in the
  // header
  available_chunk->size = size;
  available_chunk->is_available = false;
  // available_chunk->prev = available_chunk->prev; // stays the same
  available_chunk->next = new_chunk;
  // 4) adjust the next and previous pointers to "insert" the new_chunk
  // into the doubly linked list. Make sure that you check edge cases (like if 
  // the insert is at the end or the beginning)
  // 5) return the ptr to the available_chunk
  

  return available_chunk; // TODO: it might be more useful to return new_chunk
}

