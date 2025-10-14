#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include "roundrobin.h"

// #define VERBOSE 1

// These are the variable names in the given Thread struct.
// Arbitrarily, one will represent the 'next' pointer in the sched's circular 
// doubly linked list, and the other will represent the 'prev' pointer.
#define NEXT sched_one
#define PREV sched_two

// The scheduler that is the default for this package
static struct scheduler rr_publish = {
  .init=NULL, 
  .shutdown=NULL, 
  .admit=rr_admit, 
  .remove=rr_remove, 
  .next=rr_next, 
  .qlen=rr_qlen
};
scheduler MyRoundRobin = &rr_publish;

// Circular-doubly-linked-list to keep track of all of the schedulable threads.
// TODO: ask if the head pointer could also be used as the cur pointer. 
// (I DON'T THINK SO BECAUSE THEN THE INSERTS MAY BE OUT OF ORDER...)
static thread head = NULL;
static thread curr = NULL;

// Add the passed context to the scheduler’s scheduling pool.
// For round robin, this thread is added to the end of the list.
void rr_admit(thread new) {
  #ifdef VERBOSE
  printf("\n[rr_admit] ENTER\n");
  #endif

  // If there is currently nothing in the list, set both the head and the tail
  // to the new thread
  if (head == NULL) {
    // Make sure these are pointing to itself, as this is a circular
    // linked list
    new->NEXT= new;
    new->PREV = new;
    head = new;

    // The first (and only) thread should be added as the scheduler's currently
    // 'selected' thread in the pool.
    curr = new;

    return;
  }

  // The tail is going to be the head's prev thread becuase this is a circular
  // -doubly-linked-list.
  thread tail = head->PREV;
 
  // Setup the new thread's next and prev to point to the head and tail.
  new->NEXT = head;
  new->PREV = tail;

  // Update the tail->next pointer to the new thread
  tail->NEXT = new;

  // Update the head->prev  pointer to the new thread
  head->PREV = new;
}

// Remove the passed context from the scheduler’s scheduling pool.
void rr_remove(thread victim) {
  #ifdef VERBOSE
  printf("\n[rr_remove] ENTER\n");
  #endif

  // If the victim happens to be the only one in the list, then just remove
  // the head and current threads, setting them to NULL
  if (victim->NEXT == victim) {
    head = NULL;
    curr = NULL;
    return;
  }
  
  // Update the head pointer if we get rid of it.
  if (victim == head) {
    head = victim->NEXT;
  }

  // Update the cur pointer if we get rid of it.
  if (victim == curr) {
    curr = victim->NEXT;
  }

  // (victim's prev)'s next becomes (victim's next)
  victim->PREV->NEXT = victim->NEXT;
  // (victim's next)'s prev becomes (victim's prev)
  victim->NEXT->PREV = victim->PREV;

  // For my own sanity
  victim->NEXT= NULL;
  victim->PREV = NULL;
}

thread rr_next(void) {
  #ifdef VERBOSE
  printf("\n[rr_next] ENTER\n");
  #endif

  // There is no next thread if there are no threads available.
  if (curr == NULL) {
    return NULL;
  }

  // The thread we will return is the 'current' thread that we have set up.
  thread next = curr;

  // Increment the currently tracked thread the 'next' one
  curr = curr->NEXT;

  return next;
}

// Return the number of runnable threads. This will be useful for lwp wait() in
// determining if waiting makes sense.
int rr_qlen(void) {
  #ifdef VERBOSE
  printf("\n[rr_qlen] ENTER\n");
  #endif
  
  if (head == NULL)  {
    return 0;
  }

  // Start the counting at the head of the list.
  int count = 1;
  thread next = head->NEXT;

  // Keep on walking though the list while counting until you make it back 
  // to the head again (this is a circular-doubly-linked-list)
  while(next != head) {
    next = next->NEXT;
    count++;
  }

  return count;
}
