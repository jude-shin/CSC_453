#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <lwp.h> // TODO: what is the difference? <> vs ""
#include "roundrobin.h"

#define DEBUG 1

// These are the variable names in the given Thread struct.
// Arbitrarily, one will represent the 'next' pointer in the lib's doubly
// linked list, and the other will represent the 'prev' pointer.
#define NEXT lib_one
#define PREV lib_two 

// The size of the RLIMIT_STACK in bytes if none is set.
// TODO: set to 8MB
#define RLIMIT_STACK_DEFAULT 8000000

// The scheduler that the package is currently using to manage the thread
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

// Queue of waiting threads
// I DON'T KNOW WHAT IS GOING ON HERE
static thread waiting = NULL;

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

// Gets the size of the stack that we should use.
// If any of the system calls error, then the return value is -1, and should
// be handled in function who called get_stacksize()
static size_t get_stacksize() {
  struct rlimit rlim;
  rlim_t limit = 0;

  if(getrlimit(RLIMIT_STACK, &rlim) == -1 || rlim.rlim_cur == RLIM_INFINITY) {
    // Set the default to RLIMIT_STACK_DEFAULT
    limit = RLIMIT_STACK_DEFAULT;
  }
  else {
    limit = rlim.rlim_cur;
  }
  
  // Ensure that the limit is going to be set to a multiple of the page size.
  // This is in bytes.
  size_t page_size = sysconf(_SC_PAGE_SIZE);

  // Catches -1 on error, as well as the page size being zero.
  if (page_size <= 0) {
    perror("[get_stacksize] Error when getting _SC_PAGE_SIZE.");
    return -1;
  }
  
  // Round the stack size to the nearest page_size.
  uintptr_t remainder = (uintptr_t)limit%(uintptr_t)page_size;

  if (remainder == 0) {
    return page_size;
  }

  // Return the limit rounded to the nearest page_size.
  return (size_t)((uintptr_t)limit + ((uintptr_t)page_size - remainder));
}


// Creates a new lightweight process which executes the given function
// with the given argument.
// lwp create() returns the (lightweight) thread id of the new thread
// or NO_THREAD if the thread cannot be created.
tid_t lwp_create(lwpfun function, void *argument){
  #ifdef DEBUG
  printf("[debug] lwp_create\n");
  #endif

  thread new = {0};

  // Get the soft stack size.
  size_t new_stacksize = get_stacksize();
  if (new_stacksize == -1) {
    perror("[lwp_create] Error when getting RLIMIT_STACK.");
    return NO_THREAD;
  }
  new->stacksize = new_stacksize;
  
  // mmap() a new stack for this thread. 
  void *new_stack = mmap(
      NULL, 
      new_stacksize, 
      PROT_READ|PROT_WRITE, 
      MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, 
      -1, 
      0);

  if (new_stack == MAP_FAILED) {
    perror("[lwp_create] Error when mmapp()ing a new stack.");
    return NO_THREAD;
  }
  new->stack = new_stack;

  // Create a new id (just using a counter)
  new->tid = tid_counter++;

  // Load the current registers into the current state.
  // TODO: do I even need to do this? 
  // swap_rfiles(NULL, &new->state);

  // Indicate that it is a live and running process.
  new->status = LWP_LIVE;

  // TODO: For my own sanity, but probably okay to remove later
  new->lib_one = NULL;
  new->lib_two = NULL;
  new->sched_one = NULL;
  new->sched_two = NULL;

  // TODO: is this the thread that created this thread?
  // in start's case, this would be the current thread, but in lwp_create, 
  // ANY thread can create a thread...
  new->exited = NULL; // TODO: I still don't know what the hell this is

  // Add this to the rolling global list of items
  lwp_add_live(new);

  // Admit the newly created "main" thread to the current scheduler
  curr_sched->admit(new);

  return new->tid;
}


// Starts the LWP system. Converts the calling thread into a LWP
// and lwp yield()s to whichever thread the scheduler chooses.
void lwp_start(void){
  #ifdef DEBUG
  printf("[debug] lwp_start\n");
  #endif
  // Setup the context for the very first thead (using the original system
  // thread).

  thread new = {0};

  // Get the soft stack size.
  size_t new_stacksize = get_stacksize();
  if (new_stacksize == -1) {
    perror("[lwp_start] Error when getting RLIMIT_STACK.");
    exit(EXIT_FAILURE);
  }
  new->stacksize = new_stacksize;

  // Use the current stack for this special thread only.
  new->stack = NULL; 
  
  // Create a new id (just using a counter)
  new->tid = tid_counter++;

  // Load the current registers into the current state.
  // TODO: do I even need to do this?
  swap_rfiles(NULL, &new->state);

  // Indicate that it is a live and running process.
  new->status = LWP_LIVE;

  // TODO: For my own sanity, but probably okay to remove later
  new->lib_one = NULL;
  new->lib_two = NULL;
  new->sched_one = NULL;
  new->sched_two = NULL;

  // TODO: is this the thread that created this thread?
  // in start's case, this would be the current thread, but in lwp_create, 
  // ANY thread can create a thread...
  new->exited = NULL; // TODO: I still don't know what the hell this is

  // Add this to the rolling global list of items
  lwp_add_live(new);

  // Admit the newly created "main" thread to the current scheduler
  curr_sched->admit(new);

  // Start the yielding process.
  lwp_yield();
}

// Yields control to another LWP. Which one depends on the sched-
// uler. Saves the current LWP’s context, picks the next one, restores
// that thread’s context, and returns. If there is no next thread, ter-
// minates the program.
void lwp_yield(void) {
  #ifdef DEBUG
  printf("[debug] lwp_yield\n");
  #endif

  // Get the thread that the scheduler gives next.
  scheduler sched = lwp_get_scheduler();
  thread next = sched->next();
  if (next == NULL) {
    exit(curr->status); // TODO: I have a feeling this does not work
  }

  // Save the current register values to curr->state
  // Load next->state to the current register values
  // TODO: check to see if this is how we really reference these states -> vs .
  swap_rfiles(&curr->state, &next->state);

  // The current thread is now the new thread the scheduler just chose.
  curr = next;
}

// Terminates the current LWP and yields to whichever thread the
// scheduler chooses. lwp exit() does not return.
void lwp_exit(int exitval) {
  #ifdef DEBUG
  printf("[debug] lwp_exit\n");
  #endif

  // Remove from the scheduler?
  scheduler sched = lwp_get_scheduler();
  sched->remove(curr);

  // Remove the lwp from the live stack.
  lwp_remove(curr);

  // Combine the status and the exitval, and set it as the thread's new status.
  curr->status = MKTERMSTAT(curr->status, exitval);

  // Add the current thread to the queue of terminated threads.
  lwp_add_term(curr);

  // The curr thread should be handled in lwp_yield(), however, this is more
  // for my sanity.
  // TODO: Maybe this is not good to have...
  // curr = NULL;

  // Yield to the next thread that the scheduler chooses.
  lwp_yield();
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
