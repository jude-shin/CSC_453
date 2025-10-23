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

/* Initalize the Global Variables */
int lifetime;
int philosophers[NUM_PHILOSOPHERS];
int phil_i[NUM_PHILOSOPHERS];
int forks[NUM_PHILOSOPHERS];
sem_t fork_sems[NUM_PHILOSOPHERS];
sem_t print_sem;

int main (int argc, char *argv[]) {
  /* index, error no, result, and parsed lifecycle. */
  int i, err, res, l;

  /* A list of threads ids for the parent to wait upon. */
  pthread_t thread_ids[NUM_PHILOSOPHERS];

  /* How many times each philosopher should go though their eat-think 
     lifecycle before exiting. The default is 3. */
  lifetime = 3; 

  /* The only (optional) command line argument is to change the lifecycle of a 
     philosopher. */
  err = 0;
  for (i=2; i<argc; i++) {
    fprintf(stderr,"%s: too many args!\n",argv[i]);
    err++;
  }
  if (argc == 2) {
    /* The lifetime argument that the user supposedly passes in. This should 
       be an integer. */
    l = atoi(argv[1]);
    if (l == 0) {
      fprintf(stderr, "[main] error parsing lifetime argument.");
      err++;
    }

    /* Change the default lifetime of a philosopher. */
    lifetime = l;
  }
  if (err) {
    fprintf(stderr,"usage: %s [n]\n",argv[0]);
    fprintf(stderr, "n: lifecycle of a philosopher.\n");
    exit(err);
  }

  /* This situation does not work with one or zero philosophers... */
  if (NUM_PHILOSOPHERS <= 1) {
    fprintf(stderr, "[main] You need at least 2 philosophers!");
    exit(EXIT_FAILURE);
  }

  /* Sets the seed for the prng. */
  set_seed();

  /* Initalizes the philosophers, forks, and semaphores, setting them to their
     inital values. */
  set_table();

  /* Print the header that shows the labels on the philosophers. */
  print_break_line();
  print_name_line();
  print_break_line();


  /* Create a pthread for each of the philosophers, having them execute the
     dine function. The pointer passed in is the address to the location of
     memory that holds an integer. Since you can only pass in void*, this will
     be cast in the dine function. */
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
  

  /* Wait for all of the threads to finish. */
  for (i=0; i<NUM_PHILOSOPHERS; i++) {
    res = pthread_join(thread_ids[i], NULL);
    if (res != 0) {
      fprintf(stderr, "[main] error joining pthred %d. errno %d", i, res);
      exit(EXIT_FAILURE);
    }
  }
  
  /* Destroy all of the semaphores. */
  clean_table();
  print_break_line();

  exit(EXIT_SUCCESS);
}

