#include <stdio.h>
#include <stdlib.h>
#include "table.h"
#include "status.h"
#include "dine.h"

/* Prints the right end of a single philosophers's status. The result is strung
   together later in print_status_line(). */
void print_status(int stat);

/* Prints a line of "=" with occasional "|" for the number of philosophers. 
   @param void.
   @return void. */
void print_break_line(void) {
  /* index values. */
  int i, j;

  printf("|");
  for (i=0; i<NUM_PHILOSOPHERS; i++) {
    for (j=0; j<col_width; j++) {
      printf("=");
    }
    printf("|");
  }
  printf("\n");
}

/* Prints all the ASCII names of the philosophers in order based on their index
   (ex. 0->'A', 1->'B'...) 
   @param void.
   @return void. */
void print_name_line(void) {
  /* index values. */
  int i, j;
  /* Number of spaces on the left or right of the label. */
  int left_padding, right_padding;

  left_padding = col_width/2;
  right_padding = col_width-left_padding-1;

  printf("|");
  for (i=0; i<NUM_PHILOSOPHERS; i++) {
    char label = get_label(i);
    /* Print the spaces on the left side of the label. */
    for (j=0; j<left_padding; j++) {
      printf(" ");
    }

    printf("%c", label);
    /* Print the remaining spaces on the right side of the label. */
    for (j=0; j<right_padding; j++) {
      printf(" ");
    }
    printf("|");
  }
  printf("\n");
}

/* Prints all philosophers' statuses on one line.
   @param void.
   @return void. */
void print_status_line(void) {
  /* index value. */
  int i;

  printf("|");
  /* Print the statuses of each philosopher in order. */
  for (i=0; i<NUM_PHILOSOPHERS; i++) {
    // The array holds the status of what the philosopher is doing
    print_status(i);
  }
  printf("\n");
}

/* Prints the right end of a single philosophers's status. The result is strung
   together later in print_status_line(). 
   @param i int index of the philosopher that is to be printed.
   @return void */
void print_status(int i) {
  /* index value */
  int j;

  /* The indicies of the forks sitting to the left and right of the current 
     philosopher. */
  int left, right;

  /* What the philosopher is doing. */
  int state;

  left = i;
  right = (i+1)%NUM_PHILOSOPHERS;

  printf(" ");
  for (j=0; j<NUM_PHILOSOPHERS; j++) {
    // TODO: make a macro for -1
    /* if the fork is the left or right fork, and that fork is not being
       occupied, print the index. Otherwise, we don't care... print a dash. */
    if ((left == j || right == j) && forks[j] == i) {
      printf("%d", j);
    }
    else {
      printf("-");
    }
  }

  state = philosophers[i].state;
  const char *msg = "";
  switch (state) {
    case CHANGING:
      msg = CHNG_MSG;
      break;
    case EATING:
      msg = EAT_MSG;
      break;
    case THINKING:
      msg = THNK_MSG;
      break;
    default:
      fprintf(stderr, "error parding curr->doing. unknown: %s", msg);
      exit(EXIT_FAILURE);
  }
  /* Print the associated message that comes with the status. */
  printf(" %s", msg);
  printf(" |");
}
