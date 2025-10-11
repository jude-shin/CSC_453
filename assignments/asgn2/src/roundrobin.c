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
thread sched_pool_tail = NULL;
thread sched_pool_cur = NULL;

// Add the passed context to the scheduler’s scheduling pool.
// For round robin, this thread is added to the end of the list
void rr_admit(thread new) {
  // NOTE: sched_one is the NEXT pointer
  // NOTE: sched_two is the PREV pointer

  // If the tail is NULL, then the head must also be NULL
  // Set both the head and the tail to the new thread
  if (sched_pool_tail == NULL) {
    // Make sure these are pointing to NULL to indicate they are the head and 
    // tail respectively
    new->sched_one = NULL;
    new->sched_two = NULL;
    
    sched_pool_head = new;
    sched_pool_tail = new;

    // The first and only thread should be added as the scheduler's 'rr_next'
    // value as the starting point
    
    sched_pool_cur = new;
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

// Remove the passed context from the scheduler’s scheduling pool.
void rr_remove(thread victim) {
  // NOTE: sched_one is the NEXT pointer
  // NOTE: sched_two is the PREV pointer

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
  // thread cur = sched_pool_head;
  thread cur = NULL;
  thread next = sched_pool_head->sched_one;

  while(next != cur) {
    // cur = next;
    next = next->sched_one;
    count++;
  }

  return count;
}
