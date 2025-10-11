#include <stddef.h>
#include <lwp.h>
#include <roundrobin.h>

// The scheduler that the package is currently using to manage the threads
static scheduler cur_sched = NULL;

// // A global list of all threads.
// // This is just the head, but every thread is going to point to another 
// // using the two pointer variables. 
// // thread->lib_one is the pointer to the NEXT thread in the linked list
// // thread->lib_two is the pointer to the PREV thread in the lined list
// static thread* head = NULL;
// // The tail is just for convenience
// static thread* tail = NULL;

static tid_t tid_t_coutner = 0;


// Creates a new lightweight process which executes the given function
// with the given argument.
// lwp create() returns the (lightweight) thread id of the new thread
// or NO THREAD if the thread cannot be created.
tid_t lwp_create(lwpfun function, void *argument){
  tid_t_coutner = tid_t_coutner+1;

  // TODO: do some wrapper stuff with the function here?

  // create a new thread
  // TODO: fill in the new thread and the information
  rfile new_rfile = {};

   thread new_thread = {};
    new_thread->tid = tid_t_coutner;
    new_thread->stack = 0; // fix this
    new_thread->stacksize = 0;
    new_thread->state = new_rfile;
    new_thread->status = 0;
    new_thread->lib_one = NULL;
    new_thread->lib_two = NULL;
    new_thread->sched_one = NULL;
    new_thread->sched_two = NULL;
    new_thread->exited = NULL;

  // admit it to the current scheduler
  cur_sched->admit(new_thread);

  return tid_t_coutner;
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
  // 
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

  // this is where you use the last variable in the thread struct 
  // (pointer "exited")
  // i am not sure yet, but this is either an int or just a true/false variable
  // I think it is just a true false variable
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
    sched = RoundRobin;
  }
  
  // If there is no current scheduler, then there is nothing more to do but to
  // setting the pointer.
  if (cur_sched == NULL) {
    cur_sched = sched;
    return;
  }

  // // TODO: (ASK) do we init the scheduler here? or does the client code do this?
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

  // Shutdown the old scheduler
  if (cur_sched->shutdown != NULL) {
    cur_sched->shutdown();
  }
  
  // Set the currently used scheduler to the scheduler that we just created
  cur_sched = sched;
}

// Returns the pointer to the current scheduler.
scheduler lwp_get_scheduler(void) {
  // TODO: (ASK) do I want this functionality?
  // // If the current user asks for no particular scheduler, just assume that
  // // they want the default: RoundRobin
  // if (cur_sched == NULL) {
  //   cur_sched  = RoundRobin;
  // }

  return cur_sched;
}
