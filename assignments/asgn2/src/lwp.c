#include <lwp.h>

// Creates a new lightweight process which executes the given function
// with the given argument.
// lwp create() returns the (lightweight) thread id of the new thread
// or NO THREAD if the thread cannot be created.
tid_t lwp_create(lwpfun function, void *argument){
  return NULL;
}


// Starts the LWP system. Converts the calling thread into a LWP
// and lwp yield()s to whichever thread the scheduler chooses.
void lwp_start(void){

}

// Yields control to another LWP. Which one depends on the sched-
// uler. Saves the current LWP’s context, picks the next one, restores
// that thread’s context, and returns. If there is no next thread, ter-
// minates the program.
void lwp_yield(void) {
}


// Terminates the current LWP and yields to whichever thread the
// scheduler chooses. lwp exit() does not return.
void lwp_exit(int exitval) {
}

// Waits for a thread to terminate, deallocates its resources, and re-
// ports its termination status if status is non-NULL.
// Returns the tid of the terminated thread or NO THREAD.
tid_t lwp_gettid(void) {
  return NULL;
}

// Returns the tid of the calling LWP or NO THREAD if not called by a
// LWP.
thread tid2thread(tid t tid) {
  return NULL;
}

// Returns the thread corresponding to the given thread ID, or NULL
// if the ID is invalid
tid_t lwp_wait(int *status) {
  return NULL;
}

// Causes the LWP package to use the given scheduler to choose the
// next process to run. Transfers all threads from the old scheduler
// to the new one in next() order. If scheduler is NULL the library
// should return to round-robin scheduling.
// scheduler lwp get scheduler(void);
// Returns the pointer to the current scheduler.
void lwp_get_scheduler(scheduler sched) {
}
