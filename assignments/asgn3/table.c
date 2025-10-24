#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>

#include "table.h"
#include "status.h"
#include "dawdle.h"
#include "dine.h"

/* Updates and prints a philosopher and its state. */
void update_phil(int i, int new_state);
/* Updates and prints a fork and its owner. */
void update_fork(int i, int phil);

/* The function which philosopher pthreads will execute. It will go through 
   lifetime number of cycles, of eating and thinking. Then it will finish by 
   returning.
   @param ip A void pointer to the index of the philosopher.
   @return void*. Nothing in particular, it just has to return something. */
void* dine(void *pp) {
  /* The philosopher of interest. */
  Phil* phil_ptr;
  /* Index values */
  int i, j;
  /* Fork indexes to the left and right of the current philosopher. */
  int left, right;

  if (pp == NULL) {
    fprintf(stderr, "[dine] cannot be passed a NULL pointer");
    exit(EXIT_FAILURE);
  }

  /* Dereference the int* we passed in. */ 
  phil_ptr = (Phil*)pp;

  /* The philosophers index/id of all the philosophers. */
  i = phil_ptr->id;
  
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
      if (sem_wait(&fork_sems[left]) == -1) {
        fprintf(stderr, "[dine] error waiting on left fork for phil %d", i);
        exit(EXIT_FAILURE);
      }
      update_fork(left, i);

      if (sem_wait(&fork_sems[right]) == -1) {
        fprintf(stderr, "[dine] error waiting on right fork for phil %d", i);
        exit(EXIT_FAILURE);
      }
      update_fork(right, i);
    }
    /* If you are odd pick up the right first. */
    else {
      if (sem_wait(&fork_sems[right]) == -1) {
        fprintf(stderr, "[dine] error waiting on right fork for phil %d", i);
        exit(EXIT_FAILURE);
      }
      update_fork(right, i);

      if (sem_wait(&fork_sems[left]) == -1) {
        fprintf(stderr, "[dine] error waiting on left fork for phil %d", i);
        exit(EXIT_FAILURE);
      }
      update_fork(left, i);
    }

    /* You are now cleared to eat. */
    update_phil(i, EATING);
    dawdle();

    /* Go back to changing. */
    update_phil(i, CHANGING);
    dawdle();

    /* Release your forks while you are changing. */
    update_fork(right, NOBODY);
    if (sem_post(&fork_sems[right]) == -1) {
      fprintf(stderr, "[dine] error posting right fork for phil %d", i);
      exit(EXIT_FAILURE);
    }

    update_fork(left, NOBODY);
    if (sem_post(&fork_sems[left]) == -1) {
      fprintf(stderr, "[dine] error posting left fork for phil %d", i);
      exit(EXIT_FAILURE);
    }
  }
  
  return NULL;
}

/* Creates all of the semaphores. 
   @param void.
   @return void. */
void set_table(void) {
  /* index value */
  int i;

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

  /* Lock the printing semaphore so that no threads try to print at the same
     time. */
  if (sem_wait(&print_sem) == -1) {
    fprintf(stderr, "[set_table] error waiting for print semaphore");
    exit(EXIT_FAILURE);
  }
  
  /* Print the first line because we just initalized (changed) the values of
     our philosophers and our forks. */
  print_status_line();

  /* Unlock the semaphore. */
  if (sem_post(&print_sem) == -1) {
    fprintf(stderr, "[set_table] error posting print semaphore");
    exit(EXIT_FAILURE);
  }
}

/* Cleans up all of the semaphores.
   @param void.
   @return void. */
void clean_table(void) {
  /* index value */
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

/* Wraps the whole switch for a philosopher with the printing semaphore. 
   @param i integer index of the philosopher.
   @param new_state the new state.
   @return void. */
void update_phil(int i, int new_state) {
  /* Lock the printing semaphore so that no threads try to print at the same
     time. */
  if (sem_wait(&print_sem) == -1) {
    fprintf(stderr, "[update_phil] error waiting for print semaphore");
    exit(EXIT_FAILURE);
  }
  
  philosophers[i].state = new_state;
  print_status_line();

  /* Unlock the semaphore. */
  if (sem_post(&print_sem) == -1) {
    fprintf(stderr, "[update_phil] error posting print semaphore");
    exit(EXIT_FAILURE);
  }
}

/* Wraps the whole switch for a fork with the printing semaphore. 
   @param i integer index of the fork.
   @param phil the index of the philosopher who holds this fork (-1 for nobody)
   @return void. */
void update_fork(int i, int phil) {
  /* Lock the printing semaphore so that no threads try to print at the same
     time. */
  if (sem_wait(&print_sem) == -1) {
    fprintf(stderr, "[update_phil] error waiting for print semaphore");
    exit(EXIT_FAILURE);
  }
  
  forks[i] = phil;
  print_status_line();

  /* Unlock the semaphore. */
  if (sem_post(&print_sem) == -1) {
    fprintf(stderr, "[update_phil] error posting print semaphore");
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

