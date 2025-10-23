#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include "table.h"
#include "dawdle.h"
#include "dine.h"

void *test_dine(void *ip) {
  if (ip == NULL) {
    fprintf(stderr, "[test_dine] cannot be passed a NULL pointer");
    exit(EXIT_FAILURE);
  }
  int i = *(int*)ip;

  printf("[test_dine]: philosophers[%d] = %d", i, philosophers[i]);

  return NULL;
}

void *dine(void *ip) {
  if (ip == NULL) {
    fprintf(stderr, "[dine] cannot be passed a NULL pointer");
    exit(EXIT_FAILURE);
  }
  int i = *(int*)ip;

  for (int j=0; j<lifetime; j++) {
    // 1) set status to thinking
    philosophers[i] = THINKING;
    dawdle();
    
    // 2b) try to find your forks
    // wait for the first fork (if you are even, pick up left first)
    // (if you are odd, pick up the right first)
    if (i % 2 == 0) {
      // start with trying to use the LEFT fork first
      // then go ahead and try to aquire the RIGHT fork

      
      // Try to lock the semaphore
    }
    else {
      // start with trying to use the RIGHT fork first
      // then go ahead and try to aquire the LEFT fork
    }

    // 3) set status to changing
    philosophers[i] = CHANGING;
    dawdle();

    // 4) set status to eating
    philosophers[i] = EATING;
    dawdle();

    // 5a) relinquish your forks

    // 5b) unlock the semaphores

    // 6) set status to changing
    philosophers[i] = CHANGING;
    dawdle();
  }
  
  return NULL;
}

void change_status() {
  // print status
}

void set_table(void) {
  // Set all of the philosophers to CHANGING state
  for (int i=0; i<NUM_PHILOSOPHERS; i++) {
    philosophers[i] = CHANGING;
  }

  // Initalize all of the forks to be unused
  for (int i=0; i<NUM_PHILOSOPHERS; i++) {
    if (sem_init(&forks[i], 0, FALSE) == -1) {
      fprintf(stderr, "[set_table] error sem_init()ing fork %d sem", i);
      exit(EXIT_FAILURE);
    }
  }
  
  // Initalize the printing semaphore
  if (sem_init(&print, 0, FALSE) == -1) {
    fprintf(stderr, "[set_table] error sem_init()ing print sem");
    exit(EXIT_FAILURE);
  }
}

void clean_table() {
  // Destroy all of the fork semaphores
  for (int i=0; i<NUM_PHILOSOPHERS; i++) {
    if (sem_destroy(&forks[i])) {
      fprintf(stderr, "[clean_table] error sem_destroy()ing fork %d sem", i);
      exit(EXIT_FAILURE);
    }
  }

  // Destroy the print semaphore
  if (sem_destroy(&print)) {
    fprintf(stderr, "[clean_table] error sem_destroy()ing print sem");
    exit(EXIT_FAILURE);
  }
}

// Get label for the philosopher based on an i
char get_label(int id) {
  // Start at ascii character 'A'
  char c = START_CHAR;

  // Increment the ascii value a number of times
  return c + id;
}

