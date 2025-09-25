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

/////////////
//  CHUNK  //
/////////////

/*
Checks the current and next chunks, to clean up space.  If curr chunk is no
being used, and next chunk is also not being used, keep the curr header, and 
turn the rest of space into the data of the curr header. 
NOTE: this does not have to be recursive. If we update every free, then we wont
run into this issue. 
*/
Chunk *merge_next(Chunk *curr) {
  // delete curr->next in a doubly linked list
  // "merge the curr->next into the data segment of cur"
  // return curr

  // cases: (when we run into the end "tail")
  // perfect: (cur->next != NULL) && (cur->next->next != NULL)
  // curr is or is greater than the third to last element

  // semi-perfect: (cur->next != NULL) && (cur->next->next == NULL)
  // curr is the second to last element

  // im-perfect: (cur->next == NULL) && (cur->next->next == NULL)
  // curr is the last element

  if (curr->next != NULL) {
    size_t new_size = curr->size + sizeof(Chunk) + curr->next->size;
    curr->size = new_size;

    Chunk *temp = curr->next->next;

    curr->next = temp;

    if (temp != NULL) {
      temp->prev = curr;
    }
  }

  return curr;
}

/*
Checks the current and prev chunks, to clean up space.
If curr chunk is not being used, and prev chunk is also not being used, 
keep the prev header, and turn the rest of space into the data of the prev 
header. 
NOTE: this does not have to be recursive. If we update every free, then we wont
run into this issue. 
*/
Chunk *merge_prev(Chunk *curr) {
  // delete curr-> prev in a doubly linked list
  // "merge curr into the data segment of cur->prev"
  // return curr->prev

  // cases: (when we run into the beginning head)
  // perfect: (curr->prev != NULL) && (curr->prev->prev != NULL)
  // semi-perfect: (curr->prev != NULL) && (curr->prev->prev == NULL)
  // im-perfect: (curr->prev == NULL) && (curr->prev->prev == NULL)

  // or you can just call merge_next on curr_prev
  if (curr->prev != NULL) {
    size_t new_size = curr->prev->size + sizeof(Chunk) + curr->size;
    curr->prev->size = new_size;

    Chunk *temp = curr->prev->prev;

    if (temp != NULL) {
      temp->next = curr;
    }
  }
  return curr;
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
  }
  return global_head_ptr;
}

Chunk *find_chunk(Chunk *curr, void *ptr) {
  // the address of the data section of the chunk. This is what the user should
  // be passing in. This is what we returned the user when we gave them the 
  // data with alloc in the first place.
  void *curr_data = (void *)((uintptr_t)curr + sizeof(Chunk));

  if (curr_data == ptr) {
    // we have reached a valid chunk!
    return curr;
  }

  if (curr->next == NULL) {
    // if we have reached here, and we didn't find a chunk with a suitable ptr 
    // then it was the users error, and we can't be bothered.
    return NULL;
  }

  // recursively linear search through our linked list
  return find_chunk(curr->next, ptr);
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

  // 2) create a new_chunk at address (available_chunk + sizeof(Chunk) + size)
  // Chunk *new_chunk = (available_chunk + 1) + size;
  Chunk *new_chunk = (Chunk*)((uintptr_t)available_chunk + sizeof(Chunk) + size);

  // 3) populate that new header with the correct information 
  new_chunk->size = available_chunk->size - size - sizeof(Chunk);
  new_chunk->is_available = true; // takes remaining available size
  new_chunk->prev = available_chunk;
  new_chunk->next = available_chunk->next;
 
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
  size_t mod = size % 16;

  if (mod != 0) {
    size = size + (16 - mod);
  }
  return size;
}
