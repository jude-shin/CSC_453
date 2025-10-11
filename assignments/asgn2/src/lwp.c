#include <stddef.h>
#include <lwp.h>
#include <roundrobin.h>

// The scheduler that the package is currently using to manage the threads
static struct scheduler* cur_sched = NULL;

// The scheduler that is the default for this package
// TODO: add this to the roundrobin library
static struct scheduler* roundrobin = {
  NULL, 
  NULL, 
  rr_admit, 
  rr_remove, 
  rr_next, 
  rr_qlen
};

// A global list of all threads.
// This is just the head, but every thread is going to point to another 
// using the two pointer variables. 
// thread->lib_one is the pointer to the NEXT thread in the linked list
// thread->lib_two is the pointer to the PREV thread in the lined list
static struct thread* head = NULL;
// The tail is just for convenience
static struct thread* tail = NULL;


// Creates a new lightweight process which executes the given function
// with the given argument.
// lwp create() returns the (lightweight) thread id of the new thread
// or NO THREAD if the thread cannot be created.
tid_t lwp_create(lwpfun function, void *argument){
  return 0;
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
  return 0;
}

// Returns the tid of the calling LWP or NO THREAD if not called by a LWP.
thread tid2thread(tid_t tid) {
  return 0;
}

// Returns the thread corresponding to the given thread ID, or NULL
// if the ID is invalid
tid_t lwp_wait(int *status) {
  return 0;
}

// Causes the LWP package to use the given scheduler to choose the
// next process to run. Transfers all threads from the old scheduler
// to the new one in next() order. If scheduler is NULL the library
// should return to round-robin scheduling.
void lwp_set_scheduler(scheduler sched) {
  // If the two schedulers are the same, then there is no point in changing
  // and transferring all the info over. (We might get stuck in a loop).
  if (sched == cur_sched) {
    return;
  }

  // If the current user asks for no particular scheduler, just assume that
  // they want the default: RoundRobin
  if (sched == NULL) {
    sched = roundrobin;
  }
  
  // If there is no current scheduler, then there is nothing more to do but to
  // setting the pointer.
  if (cur_sched == NULL) {
    cur_sched = sched;
    return;
  }

  // TODO: do we init the scheduler here? or does the client code do this?
  // if (cur_sched->init != NULL) {
  //   cur_sched->init();
  // }

  // TODO: check that the next() returns NULL when there is nothing left
  thread nxt = NULL;
  nxt = cur_sched->next();
  while(nxt != NULL) { 
    cur_sched->remove(nxt); // Remove the thread from the old scheduler.
    sched->admit(nxt); // Add that thread to the new scheduler.
    nxt = cur_sched->next();// Onto the next thread in the old scheduler.
  }

  // Shut down the old scheduler
  if (cur_sched->shutdown != NULL) {
    cur_sched->shutdown();
  }
  
  // Set the currently used scheduler to the scheduler that we just created
  cur_sched = sched;
}

// Returns the pointer to the current scheduler.
scheduler lwp_get_scheduler(void) {
  return cur_sched;
}
