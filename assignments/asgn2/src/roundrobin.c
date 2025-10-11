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

// Can be a circular linked list
thread sched_pool_head = NULL;

// // TODO: add the tail in later
thread sched_pool_tail = NULL;

// Add the passed context to the scheduler’s scheduling pool.
// For round robin, this thread is added to the end of the list
void rr_admit(thread new) {
  // If this is the first process that is added to the scheduler, then the head
  // value should be NULL. In that case the new thread is going to be the only
  // thread.
  if (sched_pool_head == NULL) {
    sched_pool_head = new;
  }
  
  // Increment the number of threads in the schedulers pool.
  RoundRobin->qlen++;
}

// Remove the passed context from the scheduler’s scheduling pool.
void rr_remove(thread victim) {

  // Decrement the number of threads in the schedulers pool.
  RoundRobin->qlen++;
}

// Return the next thread to be run or NULL if there isn’t one.
// For round robin, iterate to the next one in the list, and loop back to the
// beginning if we reach the end of the list.
thread rr_next(void) {
  return NULL;
}

// Return the number of runnable threads. This will be useful for lwp wait() in
// determining if waiting makes sense.
int rr_qlen(void) {
  return 0;
}


// ======================================

// Tack on the new thread to the end of the globally linked list (to the tail)
// This only handles the scheduler's linked list portion
void append_sched_ll(thread new) {
  // TODO: change this in the header file
  // sched_one is the NEXT pointer
  // sched_two is the PREV pointer

  // If the tail is NULL, then the head must also be NULL
  // Set both the head and the tail to the new thread
  if (sched_pool_tail == NULL) {
    // Make sure these are pointing to NULL to indicate they are the head and 
    // tail respectively
    new->sched_one = NULL;
    new->sched_two = NULL;
    
    sched_pool_head = new;
    sched_pool_tail = new;
    return;
  }

  // new's 'next' becomes cur's 'next'
  new->sched_one = sched_pool_tail->sched_one;
  // new's 'prev' becomes cur
  new->sched_two = sched_pool_tail;

  // cur's 'next' becomes new
  sched_pool_tail->sched_one = new;
  // cur's 'prev' stays the same
  // new->sched_two = new->sched_two;
}

// Removes a victim from the lined list
// This only handles the scheduler's linked list portion
void remove_sched_ll(thread victim) {
  // TODO: change this in the header file
  // sched_one is the NEXT pointer
  // sched_two is the PREV pointer

  // (victim's prev)'s next becomes (victim's next)
  if (victim->sched_two != NULL) {
    victim->sched_two->sched_one = victim->sched_one;
  }
  // (victim's next)'s prev becomes (victim's prev)
  if (victim->sched_one != NULL) {
    victim->sched_one->sched_two = victim->sched_two;
  }
}
