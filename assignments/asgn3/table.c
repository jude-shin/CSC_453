#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>

#include "table.h"
#include "status.h"
#include "dawdle.h"
#include "dine.h"

/* The function which philosopher pthreads will execute. It will go through 
   lifetime number of cycles, of eating and thinking. Then it will finish by 
   returning.
   @param ip A void pointer to the index of the philosopher.
   @return void*. Nothing in particular, it just has to return something. */
void *dine(void *ip) {
  int i, j, left, right;

  if (ip == NULL) {
    fprintf(stderr, "[dine] cannot be passed a NULL pointer");
    exit(EXIT_FAILURE);
  }
  i = *(int*)ip;
  left = i;
  right = (i+1)%NUM_PHILOSOPHERS;


  for (j=0; j<lifetime; j++) {
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

/* Sets the global variables, and creates all of the semaphores. 
   @param void.
   @return void. */
void set_table(void) {
  /* index value */
  int i;

  /* For calculating the message lengths. */
  int chng_msg_len, eat_msg_len, thnk_msg_len;

  /* Set up philosophers, the index and the forks global variables. */
  for (i=0; i<NUM_PHILOSOPHERS; i++) {
    /* Set the philosophers to start at the changing state. */
    philosophers[i] = CHANGING;

    /* Basic index ints that dine can point to. */
    phil_i[i] = i;

    /* Set the forks' owner to -1 (nobody). */
    forks[i] = -1;
  }

  /* Calculate the width of each column to set the col_width global var. */
  chng_msg_len = strlen(CHNG_MSG);
  if (chng_msg_len == 0) {
    fprintf(stderr, "[set_table] CHNG_MSG message length cannot be 0");
    exit(EXIT_FAILURE);
  }

  eat_msg_len = strlen(EAT_MSG);
  if (eat_msg_len == 0) {
    fprintf(stderr, "[set_table] EAT_MSG message length cannot be 0");
    exit(EXIT_FAILURE);
  }

  thnk_msg_len = strlen(THNK_MSG);
  if (thnk_msg_len == 0) {
    fprintf(stderr, "[set_table] THNK_MSG message length cannot be 0");
    exit(EXIT_FAILURE);
  }

  if (!(chng_msg_len == eat_msg_len &&
        eat_msg_len == thnk_msg_len &&
        thnk_msg_len == chng_msg_len)) {
    fprintf(stderr, "[set_table] message lengths are not equal!");
    exit(EXIT_FAILURE);
  }
  /* There is one ' ' at the beginning and end, and there is NUM_PHILOSOPHERS 
     spaces for the fork statuses, a section for the message length, and 
     finally, one more ' ' separating the fork statuses and the messge. */
  col_width = 1+NUM_PHILOSOPHERS+1+(eat_msg_len)+1;


  /* Initalize all of the fork semaphores to be available. */
  for (i=0; i<NUM_PHILOSOPHERS; i++) {
    if (sem_init(&fork_sems[i], 0, 1) == -1) {
      fprintf(stderr, "[set_table] error sem_init()ing fork %d sem", i);
      exit(EXIT_FAILURE);
    }
  }
  
  /* Initalize the printing semaphore to be available. */
  if (sem_init(&print_sem, 0, 1) == -1) {
    fprintf(stderr, "[set_table] error sem_init()ing print sem");
    exit(EXIT_FAILURE);
  }
}

/* Cleans up all of the semaphores.
   @param void.
   @return void. */
void clean_table(void) {
  int i;

  /* Destroy all of the fork semaphores. */
  for (i=0; i<NUM_PHILOSOPHERS; i++) {
    if (sem_destroy(&fork_sems[i])) {
      fprintf(stderr, "[clean_table] error sem_destroy()ing fork %d sem", i);
      exit(EXIT_FAILURE);
    }
  }

  /* Destroy the print semaphore. */
  if (sem_destroy(&print_sem)) {
    fprintf(stderr, "[clean_table] error sem_destroy()ing print sem");
    exit(EXIT_FAILURE);
  }
}


/* Get ASCII label for the philosopher based on an index.
   @param id index int that the label will be based off of. 
   @return char the ASCII label that is associated with the id. */
char get_label(int id) {
  /* What the first ASCII character will be based on. */ 
  char c = START_CHAR;

  /* Increment the start character id number of times to get the new label. */ 
  return c + id;
}

