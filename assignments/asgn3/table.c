#include <stdio.h>
#include <stdlib.h>
#include "table.h"
#include "dawdle.h"
#include "status.h"
#include "dine.h"


void *test_dine(void *p) {
  Phil *curr = (Phil*)p;

  printf("[%d] dining... \n", curr->id);
  dawdle();
  printf("[%d] finished dining! \n", curr->id);

  return NULL;
}

void *dine(void *p) {
  // TODO: error check to see if this casts into a Phil?
  Phil *curr = (Phil*)p;

  for (int i=0; i<lifetime; i++) {
    // 1) set status to thinking
    curr->doing = THINKING;
    dawdle();
    
    // 2) try to find your forks
    // wait for the first fork (if you are even, pick up left first)
    // (if you are odd, pick up the right first)
    if (curr->id % 2 == 0) {
      // start with trying to use the LEFT fork first
      // then go ahead and try to aquire the RIGHT fork
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

    // 5) relinquish your forks
    // TODO: does the order in which you relinquish the forks matter?
    // 

    // 6) set status to changing
    curr->doing = CHANGING;
    dawdle();
  }
  
  return NULL;
}

void change_status(Phil *curr) {
  // print status
}

// Get label for the philosopher based on an i
char get_label(int id) {
  // Start at ascii character 'A'
  char c = START_CHAR;

  // Increment the ascii value a number of times
  return c + id;
}
