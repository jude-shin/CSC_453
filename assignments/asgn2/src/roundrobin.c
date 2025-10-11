#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include "roundrobin.h"

#define DEBUG 0

// These are the variable names in the given Thread struct.
// Arbitrarily, one will represent the 'next' pointer in the doubly linked list
// , and the other will represent the 'prev' pointer.
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
static thread sched_pool_head = NULL;
static thread sched_pool_cur = NULL;

// Add the passed context to the scheduler’s scheduling pool.
// For round robin, this thread is added to the end of the list.
void rr_admit(thread new) {
  if (DEBUG) {
    printf("fn called: rr_admit");
  }

  // If there is currently nothing in the list, set both the head and the tail
  // to the new thread
  if (sched_pool_head == NULL) {
    // Make sure these are pointing to itself, as this is a circular
    // linked list
    new->NEXT= new;
    new->PREV = new;
    sched_pool_head = new;

    // The first (and only) thread should be added as the scheduler's currently
    // 'selected' thread in the pool.
    sched_pool_cur = new;

    return;
  }

  // The tail is going to be the head's prev thread becuase this is a circular
  // -doubly-linked-list.
  thread tail = sched_pool_head->PREV;
 
  // Setup the new thread's next and prev to point to the head and tail.
  new->NEXT = sched_pool_head;
  new->PREV = tail;

  // Update the tail->next pointer to the new thread
  tail->NEXT = new;

  // Update the head->prev  pointer to the new thread
  sched_pool_head->PREV = new;
}

// Remove the passed context from the scheduler’s scheduling pool.
void rr_remove(thread victim) {
  if (DEBUG) {
    printf("fn called: rr_remove");
  }

  // If the victim happens to be the only one in the list, then just remove
  // the head and current threads, setting them to NULL
  if (victim->NEXT == victim) {
    sched_pool_head = NULL;
    sched_pool_cur = NULL;
    return;
  }
  
  // Update the head pointer if we get rid of it.
  if (victim == sched_pool_head) {
    sched_pool_head = victim->NEXT;
  }

  // Update the cur pointer if we get rid of it.
  if (victim == sched_pool_cur) {
    sched_pool_cur = victim->NEXT;
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
  if (DEBUG) {
    printf("fn called: rr_next");
  }

  // There is no next thread if there are no threads available.
  if (sched_pool_cur == NULL) {
    return NULL;
  }

  // The thread we will return is the 'current' thread that we have set up.
  thread next = sched_pool_cur;

  // Increment the currently tracked thread the 'next' one
  sched_pool_cur = sched_pool_cur->NEXT;

  return next;
}

// Return the number of runnable threads. This will be useful for lwp wait() in
// determining if waiting makes sense.
int rr_qlen(void) {
  if (DEBUG) {
    printf("fn called: rr_qlen");
  }
  
  if (sched_pool_head == NULL)  {
    return 0;
  }

  // Start the counting at the head of the list.
  int count = 1;
  thread next = sched_pool_head->NEXT;

  // Keep on walking though the list while counting until you make it back 
  // to the head again (this is a circular-doubly-linked-list)
  while(next != sched_pool_head) {
    next = next->NEXT;
    count++;
  }

  return count;
}
