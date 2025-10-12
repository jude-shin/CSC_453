#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <lwp.h> // TODO: what is the difference? <> vs ""
#include "roundrobin.h"

#define DEBUG 1

// These are the variable names in the given Thread struct.
// Arbitrarily, one will represent the 'next' pointer in the lib's doubly
// linked list, and the other will represent the 'prev' pointer.
#define NEXT lib_one
#define PREV lib_two 

// The scheduler that the package is currently using to manage the threads
static scheduler curr_sched = NULL;

// A global list of all threads.
static thread head = NULL;
// The tail is just for convenience.
static thread tail = NULL;

// Zombie threads that have been exited, but not cleaned up (waited on).
// This is a singly linked list that shares the lib pointers
static thread zombies = NULL;

// TODO: (ASK) I think this is how we are going to keep track of it. Is this
// a good way of doing this? I don't like using global variables, but I think
// it is fine since the whole point of this assignment is to have only one
// process going on at a time.
// The current thread that is in use. 
// TODO: (ASK) In most cases, is this the 'caller' thread?
static thread curr = NULL;

// A counter for all the ids. We assume the domain will never be more than
// 2^64 - 2 threads.
static tid_t tid_counter = 0;

// Calls the given lwpfun with the given argument. After it is finished with 
// the lwpfun, it valles lwp_exit() with the appropriate return value.
static void lwp_wrap(lwpfun fun, void *arg) {
  int rval;
  rval = fun(arg);
  lwp_exit(rval);
}

// Append the new thread to the end of the running list of threads
// I believe this is the key to the FIFO structure
static void lwp_append(thread new) {
  // In either case, the new thread will act as the tail, symbolized by its
  // NEXT pointer being NULL.
  new->NEXT = NULL;

  // If there is nothing allocated yet, then don't do any pointer juggling
  if (head == NULL) {
    new->PREV = NULL;
    head = new;
  }
  // Otherwise, append as normal.
  else {
    new->PREV = tail;
    tail->NEXT = new;
  }

  // In either case, when all is said and done, set the tail to be the new
  // thread.
  tail = new;
}

// Remove a thread from the global list
// This is in support of WAITED threads. Not exited threads; those become
// zombies for the watied threads to pick up.
static void lwp_remove(thread new) {
}


// Creates a new lightweight process which executes the given function
// with the given argument.
// lwp create() returns the (lightweight) thread id of the new thread
// or NO_THREAD if the thread cannot be created.
tid_t lwp_create(lwpfun function, void *argument){
  #ifdef DEBUG
  printf("[debug] lwp_create\n");
  #endif

  tid_counter++;

  // TODO: do some wrapper stuff with the function here?

  // create a new thread
  // TODO: fill in the new thread and the information
  rfile new_rfile = {};

  thread new = {};
  new->tid = tid_counter;
  new->stack = 0; // fix this
  new->stacksize = 0;
  new->state = new_rfile;
  new->status = 0;
  new->lib_one = NULL;
  new->lib_two = NULL;
  new->sched_one = NULL;
  new->sched_two = NULL;
  new->exited = NULL;

  // admit it to the current scheduler
  curr_sched->admit(new);

  return tid_counter;
  return NO_THREAD;
}


// Starts the LWP system. Converts the calling thread into a LWP
// and lwp yield()s to whichever thread the scheduler chooses.
void lwp_start(void){
  #ifdef DEBUG
  printf("[debug] lwp_start\n");
  #endif

  tid_counter++;
  unsigned long *new_stack; // THis is the current stack! (Use NULL)?
  size_t new_stacksize;
  rfile new_rfile = {};
  unsigned int new_status;
  thread lib_one;
  thread lib_two;

  // TODO: condense this
  thread new = {};
  new->tid = tid_counter;
  new->stack = new_stack;
  new->stacksize = new_stacksize;
  new->state = new_rfile;
  new->status = new_status;
  new->lib_one = lib_one;
  new->lib_two = lib_two;
  new->sched_one = NULL; // doesn't matter... sched problem to deal with 
  new->sched_two = NULL; // doesn't matter... sched problem to deal with 
  new->exited = NULL; // TODO: I still don't know what the hell this is

  // Add this to the rolling global list of items
  lwp_append(new);

  // Admit the newly created "main" thread to the current scheduler
  curr_sched->admit(new);

  lwp_yield();
  
  // TODO (1): (ASK) do I have to lwp_exit() here?
  // I don't think so... in numbers.c, there is one exit status, and I think
  // that is in reference to the "main" thread that we are creating here.
  // SO, every wrapped funciton should call lwp_exit()?
}

// Yields control to another LWP. Which one depends on the sched-
// uler. Saves the current LWP’s context, picks the next one, restores
// that thread’s context, and returns. If there is no next thread, ter-
// minates the program.
void lwp_yield(void) {
  #ifdef DEBUG
  printf("[debug] lwp_yield\n");
  #endif
}

// Terminates the current LWP and yields to whichever thread the
// scheduler chooses. lwp exit() does not return.
void lwp_exit(int exitval) {
  #ifdef DEBUG
  printf("[debug] lwp_exit\n");
  #endif

  // NOTE: no deallocation happens here. That is for lwp_wait() to handle.
}

// Returns the tid of the calling LWP or NO_THREAD if not called by a LWP.
// TODO: I don't know how to find the previously calling thread?
tid_t lwp_gettid(void) {
  #ifdef DEBUG
  printf("[debug] lwp_gettid\n");
  #endif

  if (curr == NULL) {
    return NO_THREAD;
  }

  return  curr->tid;
}

// Returns the thread corresponding to the given thread ID, or NULL
// if the ID is invalid
// TODO (2): (ASK) I am assuming that we only want threads that are not zombies. 
// aka, threads that have not terminated yet.
thread tid2thread(tid_t tid) {
  #ifdef DEBUG
  printf("[debug] tid2thread\n");
  #endif
  
  // Classic linear search of the linked list starting at the head.
  thread t = head;
  while (t != NULL) {
    if (t->tid == tid) {
      return t;
    }
    t = t->NEXT;
  }
    
  // If we have reached this point, then there is no id that matches

  return NULL;
}

// Waits for a thread to terminate, deallocates its resources, and re-
// ports its termination status if status is non-NULL.
// Returns the tid of the terminated thread or NO_THREAD.
tid_t lwp_wait(int *status) {
  #ifdef DEBUG
  printf("[debug] lwp_wait\n");
  #endif

  // search through the global list to find the 'oldest' thread. This should
  // follow FIFO, so it is whichever thread you come across 

  thread t = zombies;
  while (t != NULL) {
    // TODO: check this... I don't think this is right, but he gave it, so 
    // it must be...
    // TODO: if we are using the zombies fifo queue, then we don't have 
    // to check this as everything in the queue is already terminated
    if (LWPTERMINATED(t->status)) { 
      // Set the value of this pointer to the chosen thread's status
      // TODO: Deallocate stuff or whatever, I just think you do mmap stuff
      // TODO (3): ERROR CHECK!!! its a systemcall 
      // I guess if the free() errors, then return NULL, and set the status
      // to something meaningful? (or -1 if you are lazy)
      if (munmap(t->stack, t->stacksize) == -1) {
        // *status = errno; // OR break out of the while loop
        *status = -1;
        return NO_THREAD;
      }

      *status = t->status;
      tid_t tid = t->tid;

      // remove from the global list?

      return tid;
    }
    t = t->NEXT;
  }

  // If we have reached this state, then there are no threads that have been 
  // terminated, but not cleaned yet.
  // give the general
  *status = -1;
  return NO_THREAD;
}

// Causes the LWP package to use the given scheduler to choose the
// next process to run. Transfers all threads from the old scheduler
// to the new one in next() order. If scheduler is NULL the library
// should return to round-robin scheduling.
void lwp_set_scheduler(scheduler sched) {
  #ifdef DEBUG
  printf("[debug] lwp_set_scheduler\n");
  #endif

  // Default to MyRoundRobin
  // After this condition, sched is not going to be NULL
  if (sched == NULL) {
    sched = MyRoundRobin;
  }

  // If both are the same (and both not NULL), then don't do anything.
  if (sched == curr_sched) {
    return;
  }

  // TODO (4): (ASK) do we init the scheduler here? or does the client code do this?
  // I am pretty sure we need to because it says we must call it before any 
  // threads are added. (which is what we are going to do in a sec).
  // Initalize the scheduler before doing anything with it
  if (sched->init != NULL) {
    sched->init();
  }

  // If there is a current scheduler must transfer the threads from 
  // the curr_sched(old) to sched(new).
  // If not, skip on ahead... No need to move around threads from one scheduler
  // to another for no reason.
  if (curr_sched != NULL) {
    thread nxt = curr_sched->next();
    while(nxt != NULL) { 
      // Remove the thread from the old scheduler.
      curr_sched->remove(nxt); 

      // Add that thread to the new scheduler.
      // This automatically overwrites the next and prev pointers, so we don't
      // need to worry about that.
      sched->admit(nxt); 
    
      // Onto the next thread in the old scheduler.
      nxt = curr_sched->next();
    }

    // Shutdown the old scheduler
    if (curr_sched->shutdown != NULL) {
      curr_sched->shutdown();
    }
  }
  
  // Set the currently used scheduler to the scheduler that we just created
  curr_sched = sched;
}

// Returns the pointer to the current scheduler.
scheduler lwp_get_scheduler(void) {
  #ifdef DEBUG
  printf("[debug] lwp_get_scheduler\n");
  #endif

  // // TODO (5a): when would the client ever want this?
  // // Should I have this return NULL if there really is no scheduler?
  // // TODO (5b): (ASK) do I want this functionality?
  // if (curr_sched == NULL) {
  //   curr_sched  = MyRoundRobin;

  //   // TODO (5c): (ASK) do I want this functionality?
  //   if (curr_sched->init != NULL) {
  //     curr_sched->init();
  //   }
  // }

  return curr_sched;
}



