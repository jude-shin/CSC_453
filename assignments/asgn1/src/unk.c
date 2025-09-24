#include <unistd.h> 
#include <stddef.h> 
#include <stdbool.h>  // do I even need this?
#include <string.h>
#include <stdint.h>

#include "datastructures.h"

// size of the hunk in bytes
#define HUNK_SIZE 64000

// TODO: put in a struct with a "bytes used"/"bytes avaliable"?
static Chunk *global_head_ptr = NULL;
static Chunk *global_tail_ptr = NULL;

/////////////
//  CHUNK  //
/////////////

/*
Checks the current and next chunks, to clean up space.  If curr chunk is no
being used, and next chunk is also not being used, keep the curr header, and 
turn the rest of space into the data of the curr header. 
Repeat recursively (setting the prev header as curr in this function) 
until the end is reached, or there is a chunk that is being used.
return the address of the header whether it merged or not
*/
void *merge_next(Chunk *curr) {
  return NULL;
}

/*
Checks the current and prev chunks, to clean up space.
If curr chunk is not being used, and prev chunk is also not being used, 
keep the prev header, and turn the rest of space into the data of the prev 
header. 
Repeat recursively (setting the prev header as curr in this function)
until the end is reached, or there is a chunk that is being used.
return the address of the header whether it merged or not
*/
void *merge_prev(Chunk *curr) {
  return NULL;
}

////////////
//  HUNK  //
////////////

// get the head of the hunk
// if this is the first time you are using the hunk, then initalize it with
// all the default data
Chunk *get_head() {
  if (global_head_ptr == NULL) {
    uintptr_t floor = (uintptr_t)sbrk(0);
    sbrk(floor % 16);

    global_head_ptr = sbrk(HUNK_SIZE);

    global_head_ptr->size = HUNK_SIZE - sizeof(Chunk);
    global_head_ptr->is_available = true;
    global_head_ptr->prev= NULL;
    global_head_ptr->next = NULL;


    global_tail_ptr = global_head_ptr; 
  }
  return global_head_ptr;
}


Chunk *find_available_chunk(Chunk *curr, size_t size) {
  // start from the head and recursively search
  // this inequality allows us to guarentee that the space we will allocate
  // will have enough for the header, the data space, and the new header space
  // at the end
  if (curr->is_available && curr->size > (size + 2*sizeof(Chunk))) {
    // then we are able to allocate it!
    // this chunk is the chosen one
    return curr;
  }

  if (curr == global_tail_ptr){
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
  
  return find_available_chunk(curr->next, size);
}

Chunk *carve_chunk(Chunk *available_chunk, size_t size, bool initalize) {
  // at this point we have a chunk that can be used for the data that we want

  // 2) create a new_chunk at address (available_chunk + sizeof(Chunk) + size)
  // Chunk *new_chunk = (available_chunk + 1) + size;
  Chunk *new_chunk = (Chunk*)((uintptr_t)available_chunk + sizeof(Chunk) + size);

  // 3) populate that new header with the correct information 
  new_chunk->size = available_chunk->size - size - sizeof(Chunk);
  new_chunk->is_available = true; // takes remaining available size
  new_chunk->prev = available_chunk;
  new_chunk->next = available_chunk->next;
 
  // if we just split out of the tail pointer's free data portion, then we need
  // to set the tail to the newly split and unused portion (the new chunk)
  if (available_chunk == global_tail_ptr) {
    global_tail_ptr = new_chunk;
  }

  if (initalize) {
    void *data = (void*)((uintptr_t)available_chunk + sizeof(Chunk));
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

size_t block_size(size_t size) {
  // TODO: round thid block up to the nearest 16 bytes so it doesn't bite us
  // in the butt
  size_t remainder = 16 - (size % 16);
  return size + remainder;
}
