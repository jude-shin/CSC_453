#include <stdio.h>
#include <stdlib.h>
#include "table.h"
#include "dawdle.h"
#include "dine.h"

void *test_dine(void *p) {
  Phil *curr = (Phil*)p;

  printf("[%d] dining... \n", curr->id);
  dawdle();
  printf("[%d] finished dining! \n", curr->id);

  return NULL;
}

void *dine(void *p) {
  if (p == NULL) {
    fprintf(stderr, "[dine] dine was passed a NULL pointer");
    exit(EXIT_FAILURE);
  }

  Phil *curr = (Phil*)p;

  for (int i=0; i<lifetime; i++) {
    // 1) set status to thinking
    curr->doing = THINKING;
    dawdle();
    
    // 2b) try to find your forks
    // wait for the first fork (if you are even, pick up left first)
    // (if you are odd, pick up the right first)
    if (curr->id % 2 == 0) {
      // start with trying to use the LEFT fork first
      // then go ahead and try to aquire the RIGHT fork
      
      // Try to lock the semaphore
      curr->left->in_use = TRUE;

      curr->right->in_use = TRUE;
    }
    else {
      // start with trying to use the RIGHT fork first
      // then go ahead and try to aquire the LEFT fork
    }

    // 3) set status to changing
    curr->doing = CHANGING;
    dawdle();

    // 4) set status to eating
    curr->doing = EATING;
    dawdle();

    // 5a) relinquish your forks
    curr->left->in_use = FALSE;
    curr->right->in_use = FALSE;

    // 5b) unlock the semaphores

    // 6) set status to changing
    curr->doing = CHANGING;
    dawdle();
  }
  
  return NULL;
}

void change_status(Phil *curr) {
  // print status
}

// Mallocs and sets up the pointers for all forks and philosophers in 
// sequential order.
// @param void.
// @return a pointer to the head philosopher. All people are created equal, 
// so "head" just gives us a way to reference the table.
Phil* set_table(void) {
  Phil *head = NULL;

  // Phil *prev_phil = head;
  Fork *prev_fork = NULL; 

  for (int i=0; i < NUM_PHILOSOPHERS; i++) {
    // Calloc space for the new philosopher
    Phil *new_phil = malloc(sizeof(Phil));
    if (new_phil == NULL) {
      fprintf(stderr, "[set_table] error malloc()ing philosopher no. %d", i);
      return NULL;
    }

    Fork *new_fork = malloc(sizeof(Fork));
    if (new_phil == NULL) {
      fprintf(stderr, "[set_table] error malloc()ing fork no. %d", i);
      return NULL;
    }

    new_phil->id = i;
    new_phil->doing = CHANGING;
    new_phil->right = new_fork;
    new_phil->left = prev_fork;

    new_fork->id = i;
    new_fork->in_use = FALSE;
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

void clean_table(void) {
  // free all of the threads please
  // 
}

// Get label for the philosopher based on an i
char get_label(int id) {
  // Start at ascii character 'A'
  char c = START_CHAR;

  // Increment the ascii value a number of times
  return c + id;
}
