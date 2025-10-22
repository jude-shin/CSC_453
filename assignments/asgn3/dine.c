#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

#include "table.h"
#include "status.h"

// How many philosophers will be fighting to the death for their spagetti
// NOTE: there will be an equal number of forks as there are philosophers
// TODO: Will be shared across all files?
void set_seed(void);
Phil* init_table(void);

int main (int argc, char *argv[]) {
  // How many times each philosopher should go though their eat-think lifecycle
  // before exiting.
  int lifetime = 3; 

  // The only (optional) command line argument is to change the lifecycle of a 
  // philosopher.
  int err = 0;
  for (int i=2; i<argc; i++) {
    fprintf(stderr,"%s: too many args!\n",argv[i]);
    err++;
  }
  if (argc == 2) {
    // TODO: somehow check to see that this really is an integer
    int l = atoi(argv[1]);
    if (l == 0) {
      fprintf(stderr, "[main] error parsing lifetime argument.");
      err++;
    }
    lifetime = l;
  }
  if (err) {
    fprintf(stderr,"usage: %s [n]\n",argv[0]);
    fprintf(stderr, "n: lifecycle of a philosopher.\n");
    exit(err);
  }

  // Initalize all of the philosophers
  // TODO: is there is 1 philosopher, do we do nothing?
  if (NUM_PHILOSOPHERS <= 1) {
    fprintf(stderr, "[main] You need at least 2 philosophers!");
    exit(EXIT_FAILURE);
  }

  // PRNG INIT --------------------------------------------------------------
  // Sets the seed for the prng
  set_seed();


  // STRING MATH INIT -------------------------------------------------------

  // TODO: ask him if this is good or if I should make a macro
  // TODO: make this a seperate function
  // 1 for the leftmost padding
  // n for the number of philosophers // TODO: or is it forks?
  // 1 for dividing padding
  // msg_len for the length of it's status 
  // 1 for the rightmost padding
  int msg_len = strlen(CHNG_MSG);
  if (msg_len != 0 && 
      !(msg_len == strlen(EAT_MSG) && 
        msg_len == strlen(CHNG_MSG))) {
    fprintf(stderr, "[main] message lengths are not equal!");
  }

  int col_width = 1+NUM_PHILOSOPHERS+1+(msg_len)+1;
  
  // 1 for the leftmost "wall"
  // for each of the philosophers: add the col width + 1 for the rightmost edge
  int full_line_width = 1+(col_width+1)*NUM_PHILOSOPHERS;
  
  // PHIL/FORK INIT ---------------------------------------------------------
  // For every philosopher and fork, append them together like a regular doubly
  // linked list.
  Phil *head = init_table();
  if (head == NULL) {
    fprintf(stderr, "[main] error setting the table (head is NULL)");
    exit(EXIT_FAILURE);
  }

  // THREADS INIT -----------------------------------------------------------
  // TODO: make a list of threads
  int thread_ids[NUM_PHILOSOPHERS];
  for (int i=0; i<NUM_PHILOSOPHERS; i++) {
    thread_ids[i]=i;
  }

  // make NUM_PHILOSOPHERS times the threads
  for (int i=0; i<NUM_PHILOSOPHERS; i++) {
    if (pthread_create() == -1) {
      fprintf(stderr, "[main] error creating child %d", i);
      exit(EXIT_FAILURE);
    }
  }

  for (int i=0; i<NUM_PHILOSOPHERS; i++) {
    pthread_join();
  }
  printf("All Done!\n");
}

// Mallocs and sets up the pointers for all forks and philosophers in 
// sequential order.
// @param void.
// @return a pointer to the head philosopher. All people are created equal, 
// so "head" just gives us a way to reference the table.
Phil* init_table(void) {
  Phil *head = NULL;

  // Phil *prev_phil = head;
  Fork *prev_fork = NULL; 

  for (int i=0; i < NUM_PHILOSOPHERS; i++) {
    // Calloc space for the new philosopher
    Phil *new_phil = malloc(sizeof(Phil));
    if (new_phil == NULL) {
      fprintf(stderr, "[main] error malloc()ing philosopher no. %d", i);
      return NULL;
    }

    Fork *new_fork = malloc(sizeof(Fork));
    if (new_phil == NULL) {
      fprintf(stderr, "[main] error malloc()ing fork no. %d", i);
      return NULL;
    }

    new_phil->id = i;
    new_phil->doing = CHANGING;
    new_phil->right = new_fork;
    new_phil->left = prev_fork;

    new_fork->id = i;
    new_fork->in_use = TRUE; // TODO: check this
    new_fork->right = NULL;
    new_fork->left = new_phil; 

    if (prev_fork != NULL) {
      prev_fork->right = new_phil;
    }
    
    // Update the previous pointers for the fork and phil so the correct
    // information is updated on the next iteration of the loop
    if (head == NULL) {
      head = new_phil;
    }
    // prev_phil = new_phil;
    prev_fork = new_fork;
  }

  head->left = prev_fork;

  prev_fork->right = head;
  
  return head;
}

void set_seed(void) {
  struct timeval tp;
  if (gettimeofday(&tp, NULL) == -1) {
    fprintf(stderr, "[set_seed] error getting time of the day");
    exit(EXIT_FAILURE);
  }
  srandom(tp.tv_sec + tp.tv_usec);
}

