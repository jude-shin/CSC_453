#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

#include "table.h"
#include "dawdle.h"
#include "dine.h"

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
  if (NUM_PHILOSOPHERS <= 1) {
    fprintf(stderr, "[main] You need at least 2 philosophers!");
    exit(EXIT_FAILURE);
  }

  // PRNG INIT --------------------------------------------------------------
  // Sets the seed for the prng
  set_seed();


  // PHIL/FORK INIT ---------------------------------------------------------
  // For every philosopher and fork, append them together like a regular doubly
  // linked list.
  Phil *head = set_table();
  if (head == NULL) {
    fprintf(stderr, "[main] error setting the table (head is NULL)");
    exit(EXIT_FAILURE);
  }


  // THREADS INIT -----------------------------------------------------------
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
  }




  clean_table();

  exit(EXIT_SUCCESS);
}

