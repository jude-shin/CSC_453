#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/resource.h>
#include <stdlib.h>
#include "lwp.h"
#include "roundrobin.h"

#define DEBUG 1

// These are the variable names in the given Thread struct.
// Arbitrarily, one will represent the 'next' pointer in the lib's doubly
// linked list, and the other will represent the 'prev' pointer.
#define NEXT lib_one
#define PREV lib_two 

// The size of the RLIMIT_STACK in bytes if none is set.
#define RLIMIT_STACK_DEFAULT 8000000 // 8MB

// The scheduler that the package is currently using to manage the thread
static scheduler curr_sched = NULL;

// Global doubly linked lists.
// The term and blocked lists should be treated as queues.
// No list should contain a duplicate thread, as only two pointers are
// shared between all three of these lists.
static thread live_head = NULL;  // Holds live threads
static thread live_tail = NULL;
static thread term_head = NULL;  // Holds termiated threads
static thread term_tail = NULL;
static thread blck_head = NULL;  // Holds blocked threads
static thread blck_tail = NULL;

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

// =============================================================
// TODO: add this to a different file?

// Append the new thread to the end of a given list.
// For the termiated queue and blocked queue this function abides by the FIFO 
// requirements. As for the list of all running threads, the order in which 
// threads are added does not matter.
// WARNING: Make sure head and tail represent the same list. If not, bad things
// are going to happen...
static void lwp_list_enqueue(thread head, thread tail, thread new) {
  if ((head == NULL) ^ (tail == NULL)) {
    // This should never happen. Either they are both NULL, or both something.
    perror("[lwp_list_enqueue] mismatching tail and head pointers");
    return;
  }

  if (head == NULL && tail == NULL) {
    new->NEXT = NULL;
    new->PREV = NULL;

    head = new;
    tail = new;
    return;
  }

  new->NEXT = NULL;
  new->PREV = tail;

  tail->NEXT = new;

  tail = new;
}

// Remove a thread from either the queue of termiated threads, the queue of 
// blocked threads, or the doubly linked list of live threads. 
// To dequeue, call lwp_list_remove(head, tail, head)
static void lwp_list_remove(thread head, thread tail, thread victim) {
  if ((head == NULL) ^ (tail == NULL)) {
    // This should never happen. Either they are both NULL, or both something.
    perror("[lwp_list_dequeue] mismatching tail and head pointers");
    return;
  }

  if (head != NULL && victim != NULL) {
    if (victim->PREV != NULL) {
      victim->PREV->NEXT = victim->NEXT;
    }
    else {
      // we are at the head...
      head = victim->NEXT;
    }

    if (victim->NEXT != NULL) {
      victim->NEXT->PREV = victim->PREV;
    }
    else {
      // we are at the tail...
      tail = victim->PREV;
    }

    // For sanity, set the pointers to NULL
    victim->NEXT = NULL;
    victim->PREV = NULL;
  }
}
// =============================================================


// Gets the size of the stack that we should use.
// If any of the system calls error, then the return value is 0, and should
// be handled in function who called get_stacksize()
static size_t get_stacksize() {
  #ifdef DEBUG
  printf("[debug] get_stacksize\n");
  #endif

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
    return 0;
  }
  
  // Round the stack size to the nearest page_size.
  uintptr_t remainder = (uintptr_t)limit%(uintptr_t)page_size;

  if (remainder == 0) {
    return (size_t)limit;
  }

  // Return the limit rounded to the nearest page_size.
  return (size_t)((uintptr_t)limit + ((uintptr_t)page_size - remainder));
}


// Creates a new lightweight process which executes the given function
// with the given argument.
// lwp create() returns the (lightweight) thread id of the new thread
// or NO_THREAD if the thread cannot be created.

// TODO: what the hell do I do with this lwpfun function?
tid_t lwp_create(lwpfun function, void *argument){
  #ifdef DEBUG
  printf("[debug] lwp_create\n");
  #endif
  // Get the current scheduler
  scheduler sched = lwp_get_scheduler();

  context new_context = {};

  // Get the soft stack size.
  size_t new_stacksize = get_stacksize();
  if (new_stacksize == 0) {
    perror("[lwp_create] Error when getting RLIMIT_STACK.");
    return NO_THREAD;
  }
  new_context.stacksize = new_stacksize;
  
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

  new_context.stack = new_stack;

  // Create a new id (just using a counter)
  new_context.tid = tid_counter++;

  // Indicate that it is a live and running process.
  new_context.status = LWP_LIVE;

  // For sanity...
  new_context.lib_one = NULL;
  new_context.lib_two = NULL;
  new_context.sched_one = NULL;
  new_context.sched_two = NULL;

  // TODO: I am still slightly unsure what this does
  new_context.exited = NULL;
  
  // ----------------------------------------------------------------------

  printf("[lwp_create] setting threads\n");
  thread new_thread = &new_context;

  // Add this to the rolling global list of items
  printf("[lwp_create] adding thread to lib live list\n");
  lwp_list_enqueue(live_head, live_tail, new_thread);

  // Admit the newly created "main" thread to the current scheduler
  printf("[lwp_create] admitting new thread to scheduler\n");
  sched->admit(new_thread);

  printf("[lwp_create] exiting function\n");
  return new_thread->tid;
}


// Starts the LWP system. Converts the calling thread into a LWP
// and lwp yield()s to whichever thread the scheduler chooses.
void lwp_start(void){
  #ifdef DEBUG
  printf("[debug] lwp_start\n");
  #endif
  // Get the current scheduler
  scheduler sched = lwp_get_scheduler();

  // Setup the context for the very first thead (using the original system
  // thread).

  // TODO: I don't know what to do here. HOW DO I BUILD THE STACK
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

  // Indicate that it is a live and running process.
  new->status = LWP_LIVE;

  // TODO: For my own sanity, but probably okay to remove later
  new->lib_one = NULL;
  new->lib_two = NULL;
  new->sched_one = NULL;
  new->sched_two = NULL;

  // TODO: I am still slightly unsure what this does
  new->exited = NULL;

  // Add this to the rolling global list of items
  lwp_list_enqueue(live_head, live_tail, new);

  // Admit the newly created "main" thread to the current scheduler
  sched ->admit(new);

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
    // TODO: I have a feeling this does not work?
    // aren't we supposed to do something with qlen?
    exit(curr->status);
  }

  // Save the current register values to curr->state
  // Load next->state to the current register values
  next->state.fxsave = FPU_INIT;
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

  // Remove from the scheduler.
  scheduler sched = lwp_get_scheduler();
  sched->remove(curr);

  // Combine the status and the exitval, and set it as the thread's new status.
  curr->status = MKTERMSTAT(curr->status, exitval);

  // Add the current thread to the queue of terminated threads.
  // This also effectively removes the current thread from the live stack also.
  lwp_list_enqueue(term_head, term_tail, curr);
 
  // Do some blocking checks if there are blocked threads.
  // TODO: how do I know that curr is the thread that blck is waiting for?
  // Is this just implied that it will "just work"
  if (blck_head != NULL) {
    // Set the blocked .exited status to this current thread.
    blck_head->exited = curr;

    // Add the unblocked thread to the scheduler again.;
    sched->admit(blck_head);
    
    // Reset this to "live" by adding it back to the live list
    lwp_list_enqueue(live_head, live_tail, blck_head);
  }

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
  
  // Grab the first element of the terminated queue, following the FIFO spec.
  thread t = term_head;

  // If there are no zombies to be cleaned, either block, or return NO_THREAD
  if (t == NULL) {
    // If there are no more threads that could possibly block (if the scheduler
    // has 1 element in it).
    scheduler sched = lwp_get_scheduler();
    if (sched->qlen() == 1) {
      return NO_THREAD;
    }
  
    // We must be blocked... How sad.
    // 1) Deschedule curr
    sched->remove(curr);

    // 2) Put curr on the blocked queue (which "removes" it from the live list)
    lwp_list_enqueue(blck_head, blck_tail, curr);

    // 3) yeild to another process
    lwp_yield();
    
    // At this point, we have returned!
    // NOTE: curr->exited has been populated with the exited thread
    // 4) Remove curr from the blocked queue
    // 5) Put the curr onto the live list.
    lwp_list_enqueue(live_head, live_tail, curr);

    // 6) exited is now the next thread to deallocate (t)
    t = curr->exited;
  }
  // Remove the thread from wherever it is
  // 1) it is either the head of the terminated queue
  // 2) it is somewhere in the (beginning, middle or end) of the terminated 
  // queue (but it is chosen as the next thread to remove as it is from a 
  // blocked process.
  lwp_list_remove(term_head, term_tail, t);

  if (munmap(t->stack, t->stacksize) == -1) {
    // Something terribly wrong has happened. This syscall failed, so we
    // note the error and give up. In prod, we might try to limp along, but
    // for now, we are just bailing. 
    perror("[lwp_wait] Error munmapping lwp! Bailing now...");
    exit(EXIT_FAILURE);
  }

  // // TODO: t->status is an integer... shouldn't it always be non-NULL?
  // if (t->status != NULL) {
  //   *status = t->status;
  // }
  *status = t->status;
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
