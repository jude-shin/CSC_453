#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

#include "table.h"
#include "status.h"
#include "dine.h"

// TODO: Will be shared across all files?
Phil* init_table(void);

int main (int argc, char *argv[]) {
  // How many times each philosopher should go though their eat-think lifecycle
  // before exiting.
  lifetime = 3; 

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
  // pid_t ppid = getpid(); // TODO: get rid of this
  pthread_t thread_ids[NUM_PHILOSOPHERS];

  // Make a thread for each of the philosophers.
  Phil *curr = head; 
  for (int i=0; i<NUM_PHILOSOPHERS; i++) {
    int res = pthread_create(
        &thread_ids[i],
        NULL,
        test_dine, 
        (void*)(curr)
        );

    if (res != 0) {
      fprintf(stderr, "[main] error creating child %d. errno %d", i, res);
      exit(EXIT_FAILURE);
    }
    
    // Move onto the next Phil
    curr = curr->right->right;
  }
  

  // Wait for all of the threads to finish.
  for (int i=0; i<NUM_PHILOSOPHERS; i++) {
    int res = pthread_join(thread_ids[i], NULL);
    if (res != 0) {
      fprintf(stderr, "[main] error creating child %d. errno %d", i, res);
      exit(EXIT_FAILURE);
    }
    // printf("Parent (%d): child %d exited!\n\n", (int)ppid, i); //TODO remove
  }

  // printf("All Done!\n");
  // TODO: print the last break line?
}

