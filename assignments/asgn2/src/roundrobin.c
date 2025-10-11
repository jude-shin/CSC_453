#include <stddef.h>
#include <lwp.h>
#include <schedulers.h>
#include "roundrobin.h"

static thread *head = NULL;

// Add the passed context to the scheduler’s scheduling pool.
// For round robin, this thread is added to the end of the list
void rr_admit(thread new) {

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

