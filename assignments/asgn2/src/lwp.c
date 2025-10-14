#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/resource.h>
#include <stdlib.h>
#include "lwp.h"
#include "roundrobin.h"

// === MACROS ================================================================
// #define DEBUG 1

// These are the variable names in the given Thread struct.
// Arbitrarily, one will represent the 'next' pointer in the lib's doubly
// linked list, and the other will represent the 'prev' pointer.
#define NEXT lib_one
#define PREV lib_two 

// The size of the RLIMIT_STACK in bytes if none is set.
#define RLIMIT_STACK_DEFAULT 8000000 // 8MB
// Byte allignment for making the stacks
#define BYTE_ALLIGNMENT 16 // 16 bytes


// === GLOBAL VARIABLES (don't freak out, its unavoidable) ===================
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
static tid_t tid_counter = 1;


// === HELPER FUCNTIONS ======================================================
static void lwp_list_enqueue(thread *head, thread *tail, thread victim);
static void lwp_list_remove(thread *head, thread *tail, thread victim);
static void lwp_wrap(lwpfun fun, void *arg);
static size_t get_stacksize();


// === LWP FUCNTIONS =========================================================
// Creates a new lightweight process which executes the given function
// with the given argument.
// lwp create() returns the (lightweight) thread id of the new thread
// or NO_THREAD if the thread cannot be created.
tid_t lwp_create(lwpfun function, void *argument){
  #ifdef DEBUG
  printf("\n[lwp_create] ENTER\n");
  #endif

  // Get the current scheduler
  scheduler sched = lwp_get_scheduler();

  #ifdef DEBUG
  printf("[lwp_create] malloc new thread\n");
  #endif
  // Save the context somewhere that persists against toggling
  thread new = malloc(sizeof(context));
  if (new == NULL) {
    perror("[lwp_create] Error when getting malloc()ing a new thread.");
    return NO_THREAD;
  }

  // Get the soft stack size.
  size_t new_stacksize = get_stacksize();
  if (new_stacksize == 0) {
    perror("[lwp_create] Error when getting RLIMIT_STACK.");
    return NO_THREAD;
  }
  new->stacksize = new_stacksize;

  #ifdef DEBUG
  printf("[lwp_create] mmap new stack\n");
  #endif
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
  // NOTE: this is not the real stack pointer... this is just used for the
  // unmmap() cleaning process. Stacks grow from high -> low addresses.
  new->stack = new_stack;

  #ifdef DEBUG
  printf("[lwp_create] setting up the rfile\n");
  #endif


  // ======================================================================== 
 
  // TODO: make sure the byte allignment is correct
  // I think leave and ret belong to it's own stack frame?

  // Calculate the top of the stack (highest address)
  unsigned long *stack_top = (unsigned long *)((char *)new_stack + new_stacksize);
  
  // Push a dummy return address for lwp_wrap (it will never return, but needs one)
  stack_top--;
  *stack_top = 0;  // NULL - if lwp_wrap ever returns, we'll segfault obviously
  
  // Push the return address to lwp_wrap
  stack_top--;
  *stack_top = (unsigned long)lwp_wrap;
  
  // Ensure 16-byte alignment of the stack after the first return
  // When we "return" to lwp_wrap, rsp will point to the dummy address (stack_top + 1)
  // That address should be 16-byte aligned
  if (((uintptr_t)(stack_top + 1)) % BYTE_ALLIGNMENT != 0) {
    size_t misalignment = ((uintptr_t)(stack_top + 1)) % BYTE_ALLIGNMENT;
    size_t adjustment = BYTE_ALLIGNMENT - misalignment;
    
    // Rebuild with proper alignment
    stack_top = (unsigned long *)((char *)stack_top - adjustment);
    *stack_top = (unsigned long)lwp_wrap;
    stack_top--;
    *stack_top = 0;  // dummy return
    stack_top++;
  }
  
  // Registers
  // Points to the return address 
  new->state.rsp = (unsigned long)stack_top;  
  // Doesn't matter?
  new->state.rbp = (unsigned long)stack_top;

  // first argument (lwpfun) - the function
  new->state.rdi = (unsigned long)function;
  // second argument (void*) - the argument
  new->state.rsi = (unsigned long)argument;
  
  // floating point registers
  new->state.fxsave = FPU_INIT;
  
  // For my own sanity 
  new->state.rax = 0;
  new->state.rbx = 0;
  new->state.rcx = 0;
  new->state.rdx = 0;
  new->state.r8 = 0;
  new->state.r9 = 0;
  new->state.r10 = 0;
  new->state.r11 = 0;
  new->state.r12 = 0;
  new->state.r13 = 0;
  new->state.r14 = 0;
  new->state.r15 = 0;
  
  // ======================================================================== 

  // Create a new id (just using a counter)
  new->tid = tid_counter;
  tid_counter = tid_counter + 1;

  // Indicate that it is a live and running process.
  new->status = LWP_LIVE;

  // For sanity...
  new->lib_one = NULL;
  new->lib_two = NULL;
  new->sched_one = NULL;
  new->sched_two = NULL;
  new->exited = NULL;
  
  #ifdef DEBUG
  printf("[lwp_create] adding thread to lib live list\n");
  #endif
  // Add this to the rolling global list of items
  lwp_list_enqueue(&live_head, &live_tail, new);

  #ifdef DEBUG
  printf("[lwp_create] admitting new thread to scheduler\n");
  #endif
  // Admit the newly created "main" thread to the current scheduler
  sched->admit(new);

  #ifdef DEBUG
  printf("[lwp_create] EXIT\n\n");
  #endif
  return new->tid;
}

// Starts the LWP system. Converts the calling thread into a LWP
// and lwp yield()s to whichever thread the scheduler chooses.
void lwp_start(void){
  #ifdef DEBUG
  printf("\n[lwp_start] ENTER\n");
  #endif

  // Setup the context for the very first thead (using the original system
  // thread).

  // Get the current scheduler
  scheduler sched = lwp_get_scheduler();

  #ifdef DEBUG
  printf("[lwp_start] malloc new thread\n");
  #endif
  // Save the context somewhere that persists against toggling
  thread new = malloc(sizeof(context));
  if (new == NULL) {
    perror("[lwp_start] Error when malloc()ing a the original thread.");
    exit(EXIT_FAILURE);
  }

  #ifdef DEBUG
  printf("[lwp_start] original process is NOT mmap()ing a new stack\n");
  #endif
  // Use the current stack for this special thread only.
  new->stack = NULL; 
  new->stacksize = 0;
  
  // Create a new id (just using a counter)
  new->tid = tid_counter;
  tid_counter = tid_counter + 1;

  // Indicate that it is a live and running process.
  new->status = LWP_LIVE;
 
  new->lib_one = NULL;
  new->lib_two = NULL;
  new->sched_one = NULL;
  new->sched_two = NULL;
  new->exited = NULL;

  #ifdef DEBUG
  printf("[lwp_start] set the curr thread to be the newly created thread\n");
  #endif
  curr = new;

  #ifdef DEBUG
  printf("[lwp_start] adding thread to lib live list\n");
  #endif
  // Add this to the rolling global list of items
  lwp_list_enqueue(&live_head, &live_tail, new);

  #ifdef DEBUG
  printf("[lwp_start] admitting new thread to scheduler\n");
  #endif
  // Admit the newly created "main" thread to the current scheduler
  sched ->admit(new);

  #ifdef DEBUG
  printf("[lwp_start] yield()ing to next thread.\n");
  #endif
  // Start the yielding process.
  lwp_yield();

  #ifdef DEBUG
  printf("\n[lwp_start] EXIT\n\n");
  #endif
}

// Yields control to another LWP. Which one depends on the sched-
// uler. Saves the current LWP’s context, picks the next one, restores
// that thread’s context, and returns. If there is no next thread, ter-
// minates the program.
void lwp_yield(void) {
  #ifdef DEBUG
  printf("\n[lwp_yield] ENTER\n");
  #endif

  #ifdef DEBUG
  printf("[lwp_yield] get sched->next() thread\n");
  #endif
  // Get the thread that the scheduler gives next.
  scheduler sched = lwp_get_scheduler();
  thread next = sched->next();

  // If the scheduler has nothing more to give, then we can safely finish!
  if (next == NULL) {
    #ifdef DEBUG
    printf("[lwp_yield] the scheduler has no more \"next\" threads!\n");
    #endif

    free(curr);
    exit(curr->status);
  }

  // The current thread is now the new thread the scheduler just chose.
  #ifdef DEBUG
  printf("[lwp_yield] have the lib's curr thread point to next\n");
  #endif
  thread old = curr;
  curr = next;
  
  #ifdef DEBUG
  printf("[lwp_yield] swap_rfiles (save old registers and load next's)\n");
  #endif
  swap_rfiles(&old->state, &next->state);

  #ifdef DEBUG
  printf("[lwp_yield] EXIT\n\n");
  #endif
}

// Terminates the current LWP and yields to whichever thread the
// scheduler chooses. lwp exit() does not return.
void lwp_exit(int exitval) {
  #ifdef DEBUG
  printf("\n[lwp_exit] ENTER\n");
  #endif

  // Remove from the scheduler.
  scheduler sched = lwp_get_scheduler();
  sched->remove(curr);

  // Combine the status and the exitval, and set it as the thread's new status.
  curr->status = MKTERMSTAT(curr->status, exitval);

  // Add the current thread to the queue of terminated threads.
  // This also effectively removes the current thread from the live stack also.
  lwp_list_enqueue(&term_head, &term_tail, curr);
 
  // Do some blocking checks if there are blocked threads.
  if (blck_head != NULL) {
    thread unblocked = blck_head;

    // Remove it from the blocked queue.
    lwp_list_remove(&blck_head, &blck_tail, unblocked);

    // Set the blocked .exited status to this current thread.
    unblocked->exited = curr;

    // Add the unblocked thread to the scheduler again.;
    sched->admit(unblocked);
    
    // Reset this to "live" by adding it back to the live list
    lwp_list_enqueue(&live_head, &live_tail, unblocked);
  }

  // Yield to the next thread that the scheduler chooses.
  lwp_yield();

  #ifdef DEBUG
  printf("[lwp_exit] EXIT\n\n");
  #endif
}

// Returns the tid of the calling LWP or NO_THREAD if not called by a LWP.
tid_t lwp_gettid(void) {
  #ifdef DEBUG
  printf("\n[lwp_getid] ENTER\n");
  #endif

  if (curr == NULL) {
    return NO_THREAD;
  }

  #ifdef DEBUG
  printf("[lwp_getid] EXIT\n\n");
  #endif
  return curr->tid;
}

// Returns the thread corresponding to the given thread ID, or NULL if the ID
// is invalid
thread tid2thread(tid_t tid) {
  #ifdef DEBUG
  printf("[tid2thread] ENTER\n");
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

  #ifdef DEBUG
  printf("[tid2thread] EXIT\n\n");
  #endif

  // If we have reached this point, then there is no id that matches
  return NULL;
}

// Waits for a thread to terminate, deallocates its resources, and reports its
// termination status if status is non-NULL. Returns the tid of the terminated
// thread or NO_THREAD.
tid_t lwp_wait(int *status) {
  #ifdef DEBUG
  printf("\n[lwp_wait] ENTER\n");
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
    lwp_list_enqueue(&blck_head, &blck_tail, curr);

    // 3) yield to another process
    lwp_yield();
    
    // At this point, we have returned!
    // NOTE: curr->exited has been populated with the exited thread
    // 4) Remove curr from the blocked queue
    lwp_list_remove(&blck_head, &blck_tail, curr);

    // 5) Put the curr onto the live list.
    lwp_list_enqueue(&live_head, &live_tail, curr);

    // 6) exited is now the next thread to deallocate (t)
    t = curr->exited;
  }
  // Remove the thread from wherever it is
  // 1) it is either the head of the terminated queue
  // 2) it is somewhere in the (beginning, middle or end) of the terminated 
  // queue (but it is chosen as the next thread to remove as it is from a 
  // blocked process.
  lwp_list_remove(&term_head, &term_tail, t);

  if (t->stack != NULL && munmap(t->stack, t->stacksize) == -1) {
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

  tid_t id = t->tid;

  // free the memory malloced for the thread

  free(t);

  #ifdef DEBUG
  printf("[lwp_wait] EXIT\n\n");
  #endif

  return id;
}

// Causes the LWP package to use the given scheduler to choose the
// next process to run. Transfers all threads from the old scheduler
// to the new one in next() order. If scheduler is NULL the library
// should return to round-robin scheduling.
void lwp_set_scheduler(scheduler sched) {
  // #ifdef DEBUG
  // printf("\n[lwp_set_scheduler] ENTER\n");
  // #endif

  // Default to MyRoundRobin
  // After this condition, sched is not going to be NULL
  if (sched == NULL) {
    sched = MyRoundRobin;
  }

  // If both are the same (and both not NULL), then don't do anything.
  if (sched == curr_sched) {
    // #ifdef DEBUG
    // printf("[lwp_set_scheduler] EXIT\n\n");
    // #endif
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

  // #ifdef DEBUG
  // printf("[lwp_set_scheduler] EXIT\n\n");
  // #endif
}

// Returns the pointer to the current scheduler.
scheduler lwp_get_scheduler(void) {
  // #ifdef DEBUG
  // printf("[lwp_get_scheduler] ENTER\n");
  // #endif

  if (curr_sched == NULL) {
    curr_sched  = MyRoundRobin;
  }

  // #ifdef DEBUG
  // printf("[lwp_get_scheduler] EXIT\n\n");
  // #endif

  return curr_sched;
}


// === QUEUE HELPER FUNCTIONS ================================================
// Append the new thread to the end of a given list.
// For the termiated queue and blocked queue this function abides by the FIFO 
// requirements. As for the list of all running threads, the order in which 
// threads are added does not matter.
// WARNING: Make sure head and tail represent the same list. If not, bad things
// are going to happen...
static void lwp_list_enqueue(thread *head, thread *tail, thread new) {
  if ((*head == NULL) ^ (*tail == NULL)) {
    // This should never happen. Either they are both NULL, or both something.
    perror("[lwp_list_enqueue] mismatching tail and head pointers");
    return;
  }

  if (*head == NULL && *tail == NULL) {
    new->NEXT = NULL;
    new->PREV = NULL;

    *head = new;
    *tail = new;
    return;
  }

  new->NEXT = NULL;
  new->PREV = *tail;

  (*tail)->NEXT = new;

  *tail = new;
}

// Remove a thread from either the queue of termiated threads, the queue of 
// blocked threads, or the doubly linked list of live threads. 
// To dequeue, call lwp_list_remove(head, tail, head)
static void lwp_list_remove(thread *head, thread *tail, thread victim) {
  if ((*head == NULL) ^ (*tail == NULL)) {
    // This should never happen. Either they are both NULL, or both something.
    perror("[lwp_list_dequeue] mismatching tail and head pointers");
    return;
  }

  if (*head != NULL && victim != NULL) {
    if (victim->PREV != NULL) {
      victim->PREV->NEXT = victim->NEXT;
    }
    else {
      // we are at the head...
      *head = victim->NEXT;
    }

    if (victim->NEXT != NULL) {
      victim->NEXT->PREV = victim->PREV;
    }
    else {
      // we are at the tail...
      *tail = victim->PREV;
    }

    // For sanity, set the pointers to NULL
    victim->NEXT = NULL;
    victim->PREV = NULL;
  }
}


// === LWP HELPER FUNCTIONS ==================================================
// Calls the given lwpfun with the given argument. After it is finished with 
// the lwpfun, it valles lwp_exit() with the appropriate return value.
static void lwp_wrap(lwpfun fun, void *arg) {
  lwp_exit(fun(arg));
}

// Gets the size of the stack that we should use.
// If any of the system calls error, then the return value is 0, and should
// be handled in function who called get_stacksize()
static size_t get_stacksize() {
  // #ifdef DEBUG
  // printf("\n[get_stacksize] ENTER\n");
  // #endif

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
    // #ifdef DEBUG
    // printf("[get_stacksize] EXIT\n\n");
    // #endif
    return (size_t)limit;
  }

  // #ifdef DEBUG
  // printf("[get_stacksize] EXIT\n\n");
  // #endif

  // Return the limit rounded to the nearest page_size.
  return (size_t)((uintptr_t)limit + ((uintptr_t)page_size - remainder));
}
