#include <stddef.h>
#include <lwp.h>
#include <schedulers.h>

// // This is to be called before any threads are admitted to the scheduler. 
// // It’s to allow the scheduler to set up. This one is allowed to be NULL, 
// // so don’t call it if it is.
// void rr_init(void) {
// }
// 
// // This is to be called when the lwp library is done with a scheduler to allow
// // it to clean up. This, too, is allowed to be NULL, so don’t call it if it is.
// void rr_shutdown(void) {
// }

// Add the passed context to the scheduler’s scheduling pool.
// For round robin, this thread is added to the end of the list
void rr_admit(thread new) {
  scheduler sched = lwp_get_scheduler();

  // Increment the number of threads in the schedulers pool.
  sched->qlen++;
}

// Remove the passed context from the scheduler’s scheduling pool.
void rr_remove(thread victim) {

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
