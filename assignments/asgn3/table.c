#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>

#include "table.h"
#include "status.h"
#include "dawdle.h"
#include "dine.h"

/* Wraps the whole switch for a philosopher with the printing semaphore. 
   @param i integer index of the philosopher.
   @param new_state the new state.
   @return void. */
void update_phil(int i,int new_state) {
  /* Lock the printing semaphore so that no threads try to print at the same
     time. */
  sem_wait(&print_sem);

  philosophers[i] = new_state;
  print_status_line();

  /* Unlock the semaphore. */
  sem_post(&print_sem);
}

/* Wraps the whole switch for a fork with the printing semaphore. 
   @param i integer index of the fork.
   @param phil the index of the philosopher who holds this fork (-1 for nobody)
   @return void. */
void update_fork(int i, int phil) {
  /* Lock the printing semaphore so that no threads try to print at the same
     time. */
  sem_wait(&print_sem);

  forks[i] = phil;
  print_status_line();

  /* Unlock the semaphore. */
  sem_post(&print_sem);
}

/* The function which philosopher pthreads will execute. It will go through 
   lifetime number of cycles, of eating and thinking. Then it will finish by 
   returning.
   @param ip A void pointer to the index of the philosopher.
   @return void*. Nothing in particular, it just has to return something. */
void *dine(void *ip) {
  if (ip == NULL) {
    fprintf(stderr, "[dine] cannot be passed a NULL pointer");
    exit(EXIT_FAILURE);
  }
  /* Index values */
  int i, j;

  /* Fork indexes to the left and right of the current philosopher. */
  int left, right;

  /* Dereference the int* we passed in. */ 
  i = *(int*)ip;
  
  /* Calculate the left and right indexes for the forks. */
  left = i;
  right = (i+1)%NUM_PHILOSOPHERS;

  /* Cycle through lifetime number of times for one philosopher before death. */
  for (j=0; j<lifetime; j++) {
    /* Start thinking. */
    update_phil(i, THINKING);
    dawdle();

    /* Find BOTH forks before eating. */
    update_phil(i, CHANGING);
    dawdle();
    
    /* Try to eat (you need your forks first). */
    /* If you are even pick up the left first. */
    if (i % 2 == 0) {
      sem_wait(&fork_sems[left]);
      update_fork(left, i);

      sem_wait(&fork_sems[right]);
      update_fork(right, i);
    }
    /* If you are odd pick up the right first. */
    else {
      sem_wait(&fork_sems[right]);
      update_fork(right, i);
      sem_wait(&fork_sems[left]);
      update_fork(left, i);
    }

    /* You are now cleared to eat. */
    update_phil(i, EATING);
    dawdle();

    /* Go back to changing. */
    update_phil(i, CHANGING);
    dawdle();

    /* Release your forks while you are changing. */
    update_fork(right, -1);
    sem_post(&fork_sems[right]);
    update_fork(left, -1);
    sem_post(&fork_sems[left]);
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

