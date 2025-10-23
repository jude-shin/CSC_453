#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

#include "table.h"
#include "status.h"
#include "dawdle.h"
#include "dine.h"

void *test_dine(void *ip) {
  if (ip == NULL) {
    fprintf(stderr, "[test_dine] cannot be passed a NULL pointer");
    exit(EXIT_FAILURE);
  }

  int i = *(int*)ip;

  printf("[test_dine] phil of index %d has status %d\n", i, philosophers[i]);

  return NULL;
}

void *dine(void *ip) {
  if (ip == NULL) {
    fprintf(stderr, "[dine] cannot be passed a NULL pointer");
    exit(EXIT_FAILURE);
  }
  int i = *(int*)ip;
  int left = i;
  int right = (i+1)%NUM_PHILOSOPHERS;


  for (int j=0; j<lifetime; j++) {
    // 1) set status to thinking
    philosophers[i] = THINKING;
    dawdle();
    print_status_line();

    // 3) set status to changing
    philosophers[i] = CHANGING;
    dawdle();
    print_status_line();
    
    // 2b) try to find your forks
    // wait for the first fork (if you are even, pick up left first)
    // (if you are odd, pick up the right first)
    if (i % 2 == 0) {
      // start with trying to use the LEFT fork first
      // then go ahead and try to aquire the RIGHT fork
      sem_wait(&fork_sems[left]);
      forks[left] = i;

      sem_wait(&fork_sems[right]);
      forks[right] = i;
    }
    else {
      // start with trying to use the RIGHT fork first
      // then go ahead and try to aquire the LEFT fork
      sem_wait(&fork_sems[right]);
      forks[right] = i;

      sem_wait(&fork_sems[left]);
      forks[left] = i;
    }

    // 4) set status to eating
    philosophers[i] = EATING;
    dawdle();
    print_status_line();

    // 5) relinquish your forks
    forks[left] = -1;
    forks[right] = -1;
    sem_post(&fork_sems[right]);
    sem_post(&fork_sems[left]);

    // 6) set status to changing
    philosophers[i] = CHANGING;
    dawdle();
    print_status_line();
  }
  
  return NULL;
}

void change_status() {
  // print status
}

void set_table(void) {
  // Set all of the philosophers to CHANGING state
  int i;
  for (i=0; i<NUM_PHILOSOPHERS; i++) {
    // Set all the philosophers status states
    philosophers[i] = CHANGING;

    // Basic index ints that dine can point to
    phil_i[i] = i;

    // Set the forks' owner to -1 (nobody)
    forks[i] = -1;
  }

  // Initalize all of the forks to be unused
  for (i=0; i<NUM_PHILOSOPHERS; i++) {
    if (sem_init(&fork_sems[i], 0, 1) == -1) {
      fprintf(stderr, "[set_table] error sem_init()ing fork %d sem", i);
      exit(EXIT_FAILURE);
    }
  }
  
  // Initalize the printing semaphore
  if (sem_init(&print_sem, 0, 1) == -1) {
    fprintf(stderr, "[set_table] error sem_init()ing print sem");
    exit(EXIT_FAILURE);
  }
}

void clean_table() {
  int i;

  // Destroy all of the fork semaphores
  for (i=0; i<NUM_PHILOSOPHERS; i++) {
    if (sem_destroy(&fork_sems[i])) {
      fprintf(stderr, "[clean_table] error sem_destroy()ing fork %d sem", i);
      exit(EXIT_FAILURE);
    }
  }

  // Destroy the print semaphore
  if (sem_destroy(&print_sem)) {
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

