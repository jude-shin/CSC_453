#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "table.h"
#include "status.h"
#include "dine.h"

// TODO: make this a global variable
static int col_width = 0;

/* Prints the right end of a single philosophers's status. The result is strung
   together later in print_status_line(). */
void print_status(int stat);

/* Gets the col_width global variable */
int get_col_width(void);

/* Prints a line of "=" with occasional "|" for the number of philosophers. 
   @param void.
   @return void. */
void print_break_line(void) {
  /* index values. */
  int i, j;

  printf("|");
  for (i=0; i<NUM_PHILOSOPHERS; i++) {
    for (j=0; j<get_col_width(); j++) {
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

  left_padding = (get_col_width())/2;
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

/* Prints all philosophers' statuses on one line. Locked by a semaphore; this
   is the only printing function that the threads will call, so this is the
   only one we must lock.
   @param void.
   @return void. */
void print_status_line(void) {
  /* index value. */
  int i;

  /* Lock the printing semaphore so that no threads try to print at the same
     time. */
  sem_wait(&print_sem);

  printf("|");
  /* Print the statuses of each philosopher in order. */
  for (i=0; i<NUM_PHILOSOPHERS; i++) {
    // The array holds the status of what the philosopher is doing
    print_status(i);
  }
  printf("\n");
  /* Unlock the semaphore. */
  sem_post(&print_sem);
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
  int stat; 

  left = i;
  right = (i+1)%NUM_PHILOSOPHERS;

  printf(" ");
  for (j=0; j<NUM_PHILOSOPHERS; j++) {
    // TODO: make a macro for -1
    /* if the fork is the left or right fork, and that fork is not being
       occupied, print the index. Otherwise, we don't care... print a dash. */
    if ((left == j || right == j) && forks[j] != -1) {
      printf("%d", j);
    }
    else {
      printf("-");
    }
  }


  stat = philosophers[i];
  const char *msg = "";
  switch (stat) {
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

// TODO: get rid of this function
int get_col_width(void) {
  // 1 for the leftmost padding
  // n for the number of philosophers
  // 1 for dividing padding
  // msg_len for the length of it's status 
  // 1 for the rightmost padding
  if (col_width == 0) {
    int msg_len = strlen(CHNG_MSG);
    if (msg_len != 0 && 
        !(msg_len == strlen(EAT_MSG) && 
          msg_len == strlen(CHNG_MSG))) {
      fprintf(stderr, "[main] message lengths are not equal!");
    }

    col_width = 1+NUM_PHILOSOPHERS+1+(msg_len)+1;
  }

  return col_width;
}

