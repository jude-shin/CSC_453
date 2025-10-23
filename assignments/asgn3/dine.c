#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>

#include "table.h"
#include "dawdle.h"
#include "status.h"
#include "dine.h"

// Initalize the Global Variables
int lifetime;

int philosophers[NUM_PHILOSOPHERS];
int phil_i[NUM_PHILOSOPHERS];

int forks[NUM_PHILOSOPHERS];
sem_t fork_sems[NUM_PHILOSOPHERS];

sem_t print_sem;

int main (int argc, char *argv[]) {
  int i, err, res;

  // How many times each philosopher should go though their eat-think lifecycle
  // before exiting.
  lifetime = 3; 

  // The only (optional) command line argument is to change the lifecycle of a 
  // philosopher.
  err = 0;
  for (i=2; i<argc; i++) {
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
  set_table();
  print_break_line();
  print_name_line();
  print_break_line();


  // THREADS INIT -----------------------------------------------------------
  pthread_t thread_ids[NUM_PHILOSOPHERS];
  // Make a thread for each of the philosophers.
  for (i=0; i<NUM_PHILOSOPHERS; i++) {
    res = pthread_create(
        &thread_ids[i],
        NULL,
        dine, 
        (void*)&phil_i[i]
        );

    if (res != 0) {
      fprintf(stderr, "[main] error creating pthread %d. errno %d", i, res);
      exit(EXIT_FAILURE);
    }
  }
  

  // Wait for all of the threads to finish.
  for (i=0; i<NUM_PHILOSOPHERS; i++) {
    res = pthread_join(thread_ids[i], NULL);
    if (res != 0) {
      fprintf(stderr, "[main] error joining pthred %d. errno %d", i, res);
      exit(EXIT_FAILURE);
    }
  }


  clean_table();
  print_break_line();

  exit(EXIT_SUCCESS);
}

