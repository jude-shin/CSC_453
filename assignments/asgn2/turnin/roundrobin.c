#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include "roundrobin.h"

// === MACROS ================================================================

// These are the variable names in the given Thread struct.
// Arbitrarily, one will represent the 'next' pointer in the sched's circular 
// doubly linked list, and the other will represent the 'prev' pointer.
#define NEXT sched_one
#define PREV sched_two


// === HELPER FUCNTIONS ======================================================
// Adds a thread to the scheduler.
static void rr_admit(thread new);
// Removes a thread from the scheduler.
static void rr_remove(thread victim);
// Gets the next thread in line (round robin style).
static thread rr_next(void);
// Calculates the length of the scheduler.
static int rr_qlen(void);


// === GLOBAL VARIABLES ======================================================
// The actual struct that holds all of the pointers to the functions.
static struct scheduler rr_publish = {
  .init=NULL, 
  .shutdown=NULL, 
  .admit=rr_admit, 
  .remove=rr_remove, 
  .next=rr_next, 
  .qlen=rr_qlen
};

// The global RoundRobin pointer that will be referenced.
scheduler MyRoundRobin = &rr_publish;

// Circular-doubly-linked-list to keep track of all of the schedulable threads.
static thread head = NULL;

// The current thread that the scheduler is processing.
static thread curr = NULL;


// === SCHEDULER FUNCTIONS ===================================================
// Add the passed context to the scheduler's scheduling pool. For round robin,
// this thread is added to the end of the list.
// @param new The new thread that is going to be added to the pool.
// @return void.
void rr_admit(thread new) {
  // If there is currently nothing in the list, set both the head and the tail
  // to the new thread
  if (head == NULL) {
    // Close the loop on itself (it is the only thread in the list now)
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

  // Update the tail->next pointer to the new thread.
  tail->NEXT = new;

  // Update the head->prev pointer to the new thread.
  head->PREV = new;
}

// Remove the passed context from the scheduler's scheduling pool. This simply
// flushes out the scheduler pointers and fixes the edge cases.
// @param victim The thread we are removing from the schedulers pool.
// @return void.
void rr_remove(thread victim) {
  // If the victim happens to be the only one in the list, then just remove
  // the head and current threads, setting them to NULL.
  if (victim->NEXT == victim) {
    head = NULL;
    curr = NULL;

    // For my own sanity
    victim->NEXT = NULL;
    victim->PREV = NULL;
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
  victim->NEXT = NULL;
  victim->PREV = NULL;
}

// Get the next thread that the schneduler chooses. In our case, we are getting
// sequentially picking the next thread in line. Return NULL if there is
// nothing to return.
// @param void.
// @return thread The next thread in line.
thread rr_next(void) {
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

// Return the number of runnable threads.
// @param void.
// @return int The number of threads in our scheduling pool.
int rr_qlen(void) {
  printf("I have entered qlen\n");
  // If there is no head, then there are no threads present.
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
