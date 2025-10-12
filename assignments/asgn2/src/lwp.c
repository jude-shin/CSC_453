#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
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

// A global list of all threads. This list is not in any particular order; we
// are adding the threads to the list by prepending it to the live_head. 
static thread live_head = NULL;

// A queue of threads that have been exited, but not cleaned up/waited on.
// This is a singly linked list. Only append to the tail and pop from the head
// is going to occur with this list.
static thread term_head = NULL;
static thread term_tail = NULL;

// The thread that is currently in context.
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

// Append the new thread to the queue of terminated threads
// the FIFO structure.
static void lwp_add_term(thread new) {
  // The terminated list represents a singly linked list, so this is just a 
  // sanity safeguard. No elements in the singly linked list will have a prev
  // pointer.
  new->PREV = NULL;

  // In either case, the new thread will act as the tail, symbolized by its
  // NEXT pointer being NULL.
  new->NEXT = NULL;

  // If there is nothing allocated yet, then don't do any pointer juggling
  if (term_head == NULL) {
    term_head = new;
  }
  // Otherwise, append to the tail of the list.
  else {
    term_tail->NEXT = new;
  }

  // In either case, when all is said and done, set the tail to be the new
  // thread.
  term_tail = new;
}

// Prepend the new thread to the list of live threads.
// The order in which we append does not matter.
// However, unlike the terminated queue, this is a doubly linked list.
static void lwp_add_live(thread new) {
  new->PREV = NULL;
  new->NEXT = live_head;
  live_head->PREV = new;
  live_head = new;
}

// Remove a thread from either the queue of termiated threads, or the doubly
// linked list of live threads. We should only be removing the head of the 
// terminated queue.
static void lwp_remove(thread victim) {
  if (victim->PREV != NULL) {
    victim->PREV->NEXT = victim->NEXT;
  }

  if (victim->NEXT == NULL) {
    victim->NEXT->PREV = victim->PREV;
  }

  // For sanity, set the pointers to NULL
  victim->NEXT = NULL;
  victim->PREV = NULL;
}


// Creates a new lightweight process which executes the given function
// with the given argument.
// lwp create() returns the (lightweight) thread id of the new thread
// or NO_THREAD if the thread cannot be created.
tid_t lwp_create(lwpfun function, void *argument){
  #ifdef DEBUG
  printf("[debug] lwp_create\n");
  #endif
  return 0;
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
  lwp_add_live(new);

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
tid_t lwp_gettid(void) {
  #ifdef DEBUG
  printf("[debug] lwp_gettid\n");
  #endif

  if (curr == NULL) {
    return NO_THREAD;
  }

  return curr->tid;
}

// Returns the thread corresponding to the given thread ID, or NULL if the ID
// is invalid
thread tid2thread(tid_t tid) {
  #ifdef DEBUG
  printf("[debug] tid2thread\n");
  #endif
  
  // Linear search through all live threads
  thread t = live_head;
  while (t != NULL) {
    if (t->tid == tid) {
      return t;
    }
    t = t->NEXT;
  }
  
  // Linear search through all the terminated threads
  t = term_head;
  while (t != NULL) {
    if (t->tid == tid) {
      return t;
    }
    t = t->NEXT;
  }

  // If we have reached this point, then there is no id that matches
  return NULL;
}

// Waits for a thread to terminate, deallocates its resources, and reports its
// termination status if status is non-NULL. Returns the tid of the terminated
// thread or NO_THREAD.
tid_t lwp_wait(int *status) {
  #ifdef DEBUG
  printf("[debug] lwp_wait\n");
  #endif
  
  // Grab the first element of the queue, following the FIFO spec.
  thread t = term_head;

  // If there are no zombies to be cleaned, note that appropriately.
  // TODO: we are supposed to use the qlen() function?
  if (t == NULL) {
    // TODO: if there are no terminated threads, the caller of lwp_wait()
    // (the curr_thread) will have to block.
    return NO_THREAD;
  }

  // Remove the thread from the beginning of the queue (t is the beginning)
  lwp_remove(t);

  if (t->exited == NULL || t->exited->tid != t->tid) {
    // The thread t is not the main process created by lwp_start(), so we can 
    // try to munmap it's contents.
    if (munmap(t->stack, t->stacksize) == -1) {
      // Something terribly wrong has happened. This syscall failed, so we
      // note the error and give up. In prod, we might try to limp along, but
      // for now, we are just bailing. 
      perror("[lwp_wait] Error munmapping lwp! Bailing now...");
      exit(EXIT_FAILURE);
    }
  }

  // TODO: t->status is an integer... shouldn't it always be non-NULL?
  if (t->status != NULL) {
    *status = t->status;
  }
  
  // tid_t tid = t->tid; // TODO: we might want to save the variable before unmap
  return t->tid;
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

  // Init the scheduler before any theads are admit()ed
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

    // Shutdown the old scheduler only after all threads are remove()ed
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

  if (curr_sched == NULL) {
    curr_sched  = MyRoundRobin;
  }

  return curr_sched;
}
