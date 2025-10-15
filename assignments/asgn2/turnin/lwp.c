#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/resource.h>
#include <stdlib.h>
#include "lwp.h"
#include "roundrobin.h"

// === MACROS ================================================================
// These are the variable names in the given Thread struct.
// Arbitrarily, one will represent the 'next' pointer in the lib's doubly
// linked list, and the other will represent the 'prev' pointer.
#define NEXT lib_one
#define PREV lib_two 

// The size of the RLIMIT_STACK in bytes if none is set.
#define RLIMIT_STACK_DEFAULT 8000000 // 8MB

// Byte allignment for making the stacks
#define BYTE_STACK_ALIGNMENT 16 // 16 bytes


// === HELPER FUCNTIONS ======================================================
// Adds a thread to any one of the lists/queues (live, term, blck).
static void lwp_list_enqueue(thread *head, thread *tail, thread victim);
// Removes a thread to any one of the lists/queues (live, term, blck).
static void lwp_list_remove(thread *head, thread *tail, thread victim);
// A wrapper for functions that a thread will execute.
static void lwp_wrap(lwpfun fun, void *arg);
// Gets the size of the virtual stack each thread will have.
static size_t get_stacksize(void);


// === GLOBAL VARIABLES ======================================================
// The scheduler that the package is currently using to manage our threads.
static scheduler curr_sched = NULL;

// Global doubly linked lists.
// The term and blocked lists should be treated as queues.
// No list should contain a duplicate thread, as only two pointers are
// shared between all three of these lists.
static thread live_head = NULL;  // Holds live threads.
static thread live_tail = NULL;
static thread term_head = NULL;  // Holds termiated threads.
static thread term_tail = NULL;
static thread blck_head = NULL;  // Holds blocked threads.
static thread blck_tail = NULL;

// The thread that is currently in context.
static thread curr = NULL;

// A counter for all the ids. We assume the domain will never be more than
// 2^64 - 2 threads, so keeping a rolling counter is just fine.
static tid_t tid_counter = 1;


// === LWP FUCNTIONS =========================================================
// Creates a new lightweight process which executes the given function
// with the given argument (wrapped by lwp_wrap).
// @param function A lwpfun that will be executed by this thread.
// @param argument A void* to an argument.
// @return A tid_t thread id of the process that we have just created. (Or
// NO_THREAD if a thead could not be created).
tid_t lwp_create(lwpfun function, void *argument){
  // "Create" a new thread by saving the context of a thread somewhere in
  // memory. If the syscall fails, catch it and bail; something has gone wrong.
  thread new = malloc(sizeof(context));
  if (new == NULL) {
    perror("[lwp_create] Error when getting malloc()ing a new thread.");
    return NO_THREAD;
  }

  // Get the soft stack size and update the new thread's context with it. If 
  // the syscall fails, catch it and bail.
  size_t new_stacksize = get_stacksize();
  if (new_stacksize == 0) {
    perror("[lwp_create] Error when getting RLIMIT_STACK.");
    return NO_THREAD;
  }
  new->stacksize = new_stacksize;

  // mmap() a new chunk of memory for this thread. This acts as the virtual 
  // stack this thread can have. Give it read and write permissions (not
  // execute). The offset should also be zero.
  void *new_stack = mmap(
      NULL, 
      new_stacksize, 
      PROT_READ|PROT_WRITE, 
      MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, 
      -1, 
      0);

  // If the syscall fails, catch it and bail; something has gone wrong.
  if (new_stack == MAP_FAILED) {
    perror("[lwp_create] Error when mmapp()ing a new stack.");
    return NO_THREAD;
  }
  // Update the new thread's context with this pointer to the "lowest" point
  // in memory of the stack. Arithmetic is done later.
  new->stack = new_stack;

  // ========================================================================
  // Virual Stack Creation
  // This is where a lot of the magic happens. This allows for the this to be
  // "returned to", allowing for the instruction pointer to go to the function
  // we really want to be executed, with the correct arguments.

  // This is the reall "bottom" of the virtual stack. It is the highest address
  // in our mmap()ed space, and it will grow towards the lower addresses.
  uintptr_t *stack = (uintptr_t*)(new_stack + new_stacksize);

  // Offset the address we will put lwp_wrap to a multuple of the
  // BYTE_STACK_ALIGNMENT. All stack frames must be built on that boundary.
  stack = stack - (uintptr_t)(BYTE_STACK_ALIGNMENT);

  // Fill in the return address (lwp_wrap); where we will go.
  *stack = (uintptr_t)lwp_wrap;

  // Make room for the base pointer. (Contents don't matter).
  stack--;

  // Filling in the Registers
  // Stack now points to where the base pointer will be.
  new->state.rbp = (unsigned long)stack;

  // Rsp doesn't matter... we will just point it to the same spot.
  // This will return to the stackframe we just set to lwp_wait.
  new->state.rsp = (unsigned long)stack;

  // First argument (lwpfun) - the function
  new->state.rdi = (unsigned long)function;

  // Second argument (void*) - the argument
  new->state.rsi = (unsigned long)argument;

  // Floating point registers
  new->state.fxsave = FPU_INIT;

  // ========================================================================

  // Create a new id (just incrementing a counter).
  new->tid = tid_counter;
  tid_counter++; 

  // Indicate that it is a live and running process.
  new->status = LWP_LIVE;

  // For sanity these are all set to NULL. This is a habbit I picked up for 
  // debugging.
  new->lib_one = NULL;
  new->lib_two = NULL;
  new->sched_one = NULL;
  new->sched_two = NULL;
  new->exited = NULL;
  
  // Add this to the global list of live threads. The order doesn't matter: I 
  // put them on the back of the list.
  lwp_list_enqueue(&live_head, &live_tail, new);

  // Admit the newly created thread to the current scheduler.
  scheduler sched = lwp_get_scheduler();
  sched->admit(new);

  return new->tid;
}

// Starts the LWP system. Converts the calling thread (the original system
// thread) into a LWP and lwp yield()s to whichever thread the scheduler 
// chooses.
// @param void.
// @return void.
void lwp_start(void){
  // "Create" a new thread by saving the context of a thread somewhere in
  // memory. If the syscall fails, catch it and bail; something has gone wrong.
  thread new = malloc(sizeof(context));
  if (new == NULL) {
    perror("[lwp_start] Error when malloc()ing a the original thread.");
    exit(EXIT_FAILURE);
  }

  // Use the current stack for this special thread only. There is no need to
  // create a new stack.
  new->stack = NULL; 
  new->stacksize = 0;
  
  // Create a new id (just using a counter).
  new->tid = tid_counter;
  tid_counter++;

  // Indicate that it is a live and running process.
  new->status = LWP_LIVE;

  // For sanity, these are all set to NULL.
  new->lib_one = NULL;
  new->lib_two = NULL;
  new->sched_one = NULL;
  new->sched_two = NULL;
  new->exited = NULL;

  // Set the current thread to be the one we just created.
  curr = new;
  
  // Add this to the rolling global list of items.
  lwp_list_enqueue(&live_head, &live_tail, new);

  // Admit the newly created "main" thread to the current scheduler.
  scheduler sched = lwp_get_scheduler();
  sched ->admit(new);

  // Start the yielding process.
  lwp_yield();
}

// Yields control to another LWP. Which one depends on the scheduler.
// Saves the current LWP's context, picks the next one, restores that thread's
// context, and returns. If there is no next thread, it terminates the program.
// @param void.
// @return void.
void lwp_yield(void) {
  // Get the thread that the scheduler gives next.
  scheduler sched = lwp_get_scheduler();
  thread next = sched->next();

  // The scheduler has nothing more to give.
  if (next == NULL) {
    tid_t t = curr->tid;
    free(curr);
    exit(t);
  }

  // The current thread is now the new thread the scheduler just chose.
  thread old = curr;
  curr = next;
 
  // Switch contexts.
  swap_rfiles(&old->state, &next->state);
}

// Terminates the current LWP and yields to whichever thread the scheduler 
// chooses. lwp exit() does not return!
// @param exitval An int indicating the exit value.
// @return void.
void lwp_exit(int exitval) {
  // Remove from the scheduler.
  scheduler sched = lwp_get_scheduler();
  sched->remove(curr);

  // Combine the status and the exitval, and set it as the thread's new status.
  curr->status = MKTERMSTAT(curr->status, exitval);

  // Take the thread off the live list, and add the current thread to the queue
  // of terminated threads.
  lwp_list_remove(&live_head, &live_tail, curr);
  lwp_list_enqueue(&term_head, &term_tail, curr);
 
  // Do some blocking checks if there are blocked threads.
  if (blck_head != NULL) {
    // Get the oldest blocked thread that is waiting for a process to end.
    thread unblocked = blck_head;

    // Remove it from the blocked queue.
    lwp_list_remove(&blck_head, &blck_tail, unblocked);

    // Set the blocked .exited status to this current thread.
    unblocked->exited = curr;

    // Add the unblocked thread to the scheduler again.
    sched->admit(unblocked);
    
    // Reset this to "live" by adding it back to the live list.
    lwp_list_enqueue(&live_head, &live_tail, unblocked);
  }

  lwp_yield();
}

// Returns the tid of the calling LWP or NO_THREAD if not called by a LWP.
// @param void.
// @return The tid_t of the calling (current) lwp.
tid_t lwp_gettid(void) {
  if (curr == NULL) {
    return NO_THREAD;
  }
  return curr->tid;
}

// Returns the thread corresponding to the given thread ID, or NULL if the ID
// is invalid.
// @param tid The tid_t we are searching for.
// @return The thread whos tid matches the parameter (or NULL if not found).
thread tid2thread(tid_t tid) {
  // Linear search through all live threads.
  thread t = live_head;
  while (t != NULL) {
    if (t->tid == tid) {
      return t;
    }
    t = t->NEXT;
  }
  
  // Linear search through all the terminated threads.
  t = term_head;
  while (t != NULL) {
    if (t->tid == tid) {
      return t;
    }
    t = t->NEXT;
  }

  // Linear search through all the blocked threads.
  t = blck_head;
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
// @param status A pointer to an integer (or NULL) that holds the status of a
// thread. 
// @return The tid of the thread who was waited upon.
tid_t lwp_wait(int *status) {
  // Grab the first element of the terminated queue.
  thread t = term_head;

  // If there are no termiated to be cleaned, either block, or return NO_THREAD
  if (t == NULL) {
    // If there are no more threads that could possibly block (if the scheduler
    // has 1 element in it).
    scheduler sched = lwp_get_scheduler();

    // We must be blocked... How sad.
    // Deschedule the current thread.
    sched->remove(curr);

    if (sched->qlen() <= 1) {
      // TODO: what to set the status to be here?
      return NO_THREAD;
    }

    // Remove the curr thread from the live list, and put it on the blocked
    // queue.
    lwp_list_remove(&live_head, &live_tail, curr);
    lwp_list_enqueue(&blck_head, &blck_tail, curr);

    // Yield to another process.
    lwp_yield();
    
    // At this point, we have returned!
    // NOTE: curr->exited has been populated with the exited thread

    // Remove curr from the blocked queue.
    lwp_list_remove(&blck_head, &blck_tail, curr);

    // Put the curr onto the live list.
    lwp_list_enqueue(&live_head, &live_tail, curr);

    // .exited is now the next thread to deallocate (t)
    t = curr->exited;
  }

  // Remove the thread from wherever it is
  // 1) it is either the head of the terminated queue
  // 2) it is somewhere in the (beginning, middle or end) of the terminated 
  // queue (but it is chosen as the next thread to remove as it is from a 
  // blocked process.
  lwp_list_remove(&term_head, &term_tail, t);

  if (munmap(t->stack, t->stacksize) == -1) {
    // Something terribly wrong has happened. This syscall failed, so we
    // note the error and give up. In prod, we might try to limp along, but
    // for now, we are just bailing. 
    perror("[lwp_wait] Error munmapping lwp! Bailing now...");
    exit(EXIT_FAILURE);
  }

  // The parameter status can be null. Don't try to dereference a NULL pointer.
  if (status != NULL) {
    *status = t->status;
  }

  // Save the id because we are going to free the thread soon.
  tid_t id = t->tid;

  // Free the memory malloced for the thread's context.
  free(t);

  return id;
}

// Causes the LWP package to use the given scheduler to choose the next process
// to run. Transfers all threads from the old scheduler to the new one in 
// next() order. If scheduler is NULL, this defaults to the MyRoundRobin 
// scheduling.
// @param sched The new scheduler.
// @return void.
void lwp_set_scheduler(scheduler sched) {
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

  // If there is a current scheduler, we must transfer the threads from 
  // the curr_sched(old) to sched(new).
  // If not, skip on ahead... No need to move around threads from one scheduler
  // to another for no reason.
  if (curr_sched != NULL) {
    thread next = curr_sched->next();
    while(next != NULL) { 
      // Remove the thread from the old scheduler.
      curr_sched->remove(next); 

      // Add that thread to the new scheduler.
      // This automatically overwrites the next and prev pointers, so we don't
      // need to worry about that.
      sched->admit(next); 
    
      // Onto the next thread in the old scheduler.
      next = curr_sched->next();
    }

    // Shutdown the old scheduler only after all threads are remove()ed
    if (curr_sched->shutdown != NULL) {
      curr_sched->shutdown();
    }
  }
  
  // Set the currently used scheduler to the scheduler that we just created
  curr_sched = sched;
}

// Returns the pointer to the current scheduler. If there is none, it defaults
// to roundrobin.
// @param void.
// @return The scheduler that is currently being used.
scheduler lwp_get_scheduler(void) {
  if (curr_sched == NULL) {
    curr_sched = MyRoundRobin;
  }

  return curr_sched;
}


// === QUEUE HELPER FUNCTIONS ================================================
// Append the new thread to the end of a given list.
// For the termiated queue and blocked queue this function abides by the FIFO 
// requirements. As for the list of all running threads, the order in which 
// threads are added does not matter.
// WARNING: Make sure head and tail represent the same list. If not, bad things
// are going to happen...
// @param head A pointer to the global head of the list.
// @param tail A pointer to the global tail of the list.
// @param new The new thread to be added.
// @return void.
static void lwp_list_enqueue(thread *head, thread *tail, thread new) {
  if (new == NULL) {
    // This should never happen. Please always call this with a valid 
    // non-NULL thead.
    perror("[lwp_list_enqueue] thread new cannot be NULL");
    return;
  }

  if ((*head == NULL) ^ (*tail == NULL)) {
    // This should never happen. Either they are both NULL, or both something.
    // If this occurs, then someone has made a big mistake. 
    perror("[lwp_list_enqueue] mismatching tail and head pointers");
    return;
  }
  
  // There is nothing in the list, so add it and set the head/tail to the same
  // thread, updating their next and prev pointers.
  if (*head == NULL && *tail == NULL) {
    new->NEXT = NULL;
    new->PREV = NULL;

    *head = new;
    *tail = new;
    return;
  }

  // Otherwise, enqueue to the back of the line (at the tail) as normal.
  new->NEXT = NULL;
  new->PREV = *tail;
  
  // Set the tail's next value to point to the new thread.
  (*tail)->NEXT = new;
  
  // Set the value at this address to new.
  *tail = new;
}

// Remove a thread from either the queue of termiated threads, the queue of 
// blocked threads, or the doubly linked list of live threads. 
// To dequeue properly, call lwp_list_remove(head, tail, head)
// @param head A pointer to the global head of the list.
// @param tail A pointer to the global tail of the list.
// @param new The new thread to be added.
// @return void.
static void lwp_list_remove(thread *head, thread *tail, thread victim) {
  if (victim == NULL) {
    // This should never happen. Please always call this with a valid 
    // non-NULL thead.
    perror("[lwp_list_remove] thread victim cannot be NULL");
    return;
  }

  if ((*head == NULL) ^ (*tail == NULL)) {
    // This should never happen. Either they are both NULL, or both something.
    // If this occurs, then someone has made a big mistake.
    perror("[lwp_list_dequeue] mismatching tail and head pointers");
    return;
  }

  if (*head != NULL && victim != NULL) {
    if (victim->PREV != NULL) {
      // Tell victim's previous node to skip over the victim and onto the next
      // thread
      victim->PREV->NEXT = victim->NEXT;
    }
    else {
      // We are at the head. Update accordingly.
      *head = victim->NEXT;
    }

    if (victim->NEXT != NULL) {
      // Tell victim's next node to skip over the victim and onto the previous 
      // thread
      victim->NEXT->PREV = victim->PREV;
    }
    else {
      // We are at the tail. Update accordingly.
      *tail = victim->PREV;
    }

    // For sanity, set the pointers to NULL, even though we don't really care
    victim->NEXT = NULL;
    victim->PREV = NULL;
  }
}


// === LWP HELPER FUNCTIONS ==================================================
// Calls the given lwpfun with the given argument. After it is finished with 
// the lwpfun, it fills lwp_exit() with the appropriate return value.
// @param fun The lwpfun which will actually execute.
// @param arg The void* that will be the arguments for the lwpfun
// @return void.
static void lwp_wrap(lwpfun fun, void *arg) {
  lwp_exit(fun(arg));
}

// Gets the size of the stack that we should use for each thread's virtual
// stack. If any of the system calls error, then the return value is 0, and 
// should be handled in function who called get_stacksize()
// @param void.
// @return The size_t of the virtual stack we should be creating.
static size_t get_stacksize(void) {
  struct rlimit rlim;
  rlim_t limit = 0;

  // Try to get the rlimit. Set to the default if the call gives garbage.
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
