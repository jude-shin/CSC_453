#include <stddef.h>
#include <lwp.h>
#include <schedulers.h>
#include "roundrobin.h"

// The scheduler that is the default for this package
// TODO: add this to the roundrobin library
scheduler RoundRobin = {
  NULL, 
  NULL, 
  rr_admit, 
  rr_remove, 
  rr_next, 
  rr_qlen
};

// TODO: is it fine that I have a bunch of global variables?
thread sched_pool_head = NULL;
thread sched_pool_cur = NULL;

// Add the passed context to the scheduler’s scheduling pool.
// For round robin, this thread is added to the end of the list
void rr_admit(thread new) {
  // NOTE: sched_one is the NEXT pointer
  // NOTE: sched_two is the PREV pointer
  thread tail = sched_pool_head->sched_two;

  // If the tail is NULL, then the head must also be NULL
  // Set both the head and the tail to the new thread
  if (tail == NULL) {
    // Make sure these are pointing to itself, as this is a circular
    // linked list
    new->sched_one = new;
    new->sched_two = new;
    sched_pool_head = new;

    // The first and only thread should be added as the scheduler's 'rr_next'
    // value as the starting point
    sched_pool_cur = new;

    return;
  }
 
  // Setup the new thread's next and prev to point to the head and tail.
  new->sched_one = sched_pool_head;
  new->sched_two = tail;

  // Update the tail->next pointer to the new thread
  tail->sched_one = new;

  // Update the head->prev  pointer to the new thread
  sched_pool_head->sched_two = new;
}

// Remove the passed context from the scheduler’s scheduling pool.
void rr_remove(thread victim) {
  // NOTE: sched_one is the NEXT pointer
  // NOTE: sched_two is the PREV pointer

  // TODO: do a check to see if it is the only item in the list
  // TODO: check to see if this is the current thread?

  // (victim's prev)'s next becomes (victim's next)
  if (victim->sched_two != NULL) {
    victim->sched_two->sched_one = victim->sched_one;
  }
  // (victim's next)'s prev becomes (victim's prev)
  if (victim->sched_one != NULL) {
    victim->sched_one->sched_two = victim->sched_two;
  }
}

// Return the next thread to be run or NULL if there isn’t one.
// For round robin, iterate to the next one in the list, and loop back to the
// beginning if we reach the end of the list.
thread rr_next(void) {
  if (sched_pool_cur == NULL) {
    return NULL;
  }
  thread next = sched_pool_cur;

  // Increment the next pointer
  sched_pool_cur = sched_pool_cur->sched_one;

  return next;
}

// Return the number of runnable threads. This will be useful for lwp wait() in
// determining if waiting makes sense.
int rr_qlen(void) {
  if (sched_pool_head == NULL)  {
    return 0;
  }
 
  int count = 1;
  thread cur = sched_pool_head;
  thread next = sched_pool_head->sched_one;

  while(next != cur) {
    cur = next;
    next = next->sched_one;
    count++;
  }

  return count;
}
