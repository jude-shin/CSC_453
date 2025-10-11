#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include "roundrobin.h"

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

// we can honestly make the head pointer the 'current' pointer
static thread sched_pool_head = NULL;
static thread sched_pool_cur = NULL;

// Add the passed context to the scheduler’s scheduling pool.
// For round robin, this thread is added to the end of the list
void rr_admit(thread new) {
  // printf("fn called: rr_admit");
  // NOTE: sched_one is the NEXT pointer
  // NOTE: sched_two is the PREV pointer

  // If there is currently nothing in the list, set both the head and the tail
  // to the new thread
  if (sched_pool_head == NULL) {
    // Make sure these are pointing to itself, as this is a circular
    // linked list
    new->sched_one = new;
    new->sched_two = new;
    sched_pool_head = new;

    // The first and only thread should be added as the scheduler's thread pool
    sched_pool_cur = new;

    return;
  }

  thread tail = sched_pool_head->sched_two;
 
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
  // printf("fn called: rr_remove");
  // NOTE: sched_one is the NEXT pointer
  // NOTE: sched_two is the PREV pointer

  // If the victim happens to be the only one in the list, then just remove
  // the head and current threads, setting them to NULL
  if (victim->sched_one == victim) {
    sched_pool_head = NULL;
    sched_pool_cur = NULL;
    return;
  }
  
  // Update the head pointer if we get rid of it.
  if (victim == sched_pool_head) {
    sched_pool_head = victim->sched_one;
  }

  // Update the cur pointer if we get rid of it.
  if (victim == sched_pool_cur) {
    sched_pool_cur = victim->sched_one;
  }

  // (victim's prev)'s next becomes (victim's next)
  victim->sched_two->sched_one = victim->sched_one;
  // (victim's next)'s prev becomes (victim's prev)
  victim->sched_one->sched_two = victim->sched_two;

  // For my own sanity
  victim->sched_one = NULL;
  victim->sched_two = NULL;
}

thread rr_next(void) {
  // printf("fn called: rr_next");

  if (sched_pool_cur == NULL) {
    return NULL;
  }

  if (sched_pool_cur->sched_one == NULL) {
    return NULL;
  }

  // Increment the next pointer
  thread next = sched_pool_cur;
  sched_pool_cur = sched_pool_cur->sched_one;

  return next;
}

// Return the number of runnable threads. This will be useful for lwp wait() in
// determining if waiting makes sense.
int rr_qlen(void) {
  // printf("fn called: rr_qlen");

  if (sched_pool_head == NULL)  {
    return 0;
  }
 
  int count = 1;
  thread next = sched_pool_head->sched_one;

  while(next != sched_pool_head) {
    next = next->sched_one;
    count++;
  }

  return count;
}
