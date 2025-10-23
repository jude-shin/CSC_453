#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "table.h"
#include "status.h"
#include "dine.h"

// TODO: test that all of the strings that are

static int col_width = 0;

// Prints a line of "=" with occasional "|" for the number of philosophers
void print_break_line(void) {
  printf("|");
  for (int i=0; i<NUM_PHILOSOPHERS; i++) {
    for (int j=0; j<get_col_width(); j++) {
      printf("=");
    }
    printf("|");
  }
  printf("\n");
}

void print_name_line(void) {
  int left_padding = (get_col_width())/2;
  int right_padding = col_width-left_padding-1;

  printf("|");

  for (int i=0; i<NUM_PHILOSOPHERS; i++) {
    char label = get_label(i);
    for (int j=0; j<left_padding; j++) {
      printf(" ");
    }

    printf("%c", label);

    for (int j=0; j<right_padding; j++) {
      printf(" ");
    }
    printf("|");
  }
  printf("\n");
}

// Prints ALL the statuses 
void print_status_line(void) {
  sem_wait(&print_sem);
  printf("|");
  for (int i=0; i<NUM_PHILOSOPHERS; i++) {
    // The array holds the status of what the philosopher is doing
    print_status(i);
  }
  printf("\n");
  sem_post(&print_sem);
}

// prints just one status
void print_status(int i) {
  printf(" ");

  int left = i;
  int right = (i+1)%NUM_PHILOSOPHERS;

  for (int j=0; j<NUM_PHILOSOPHERS; j++) {
    // if we are printing the left fork, see if it is occupied by the current
    // philosopher (the fork's value will be the index of the philosopher 
    // that is using it)
    // TODO: make this more efficient
    if (left == j && forks[left] != -1) {
      printf("%d", j);
    }
    else if (right == j && forks[right] != -1) {
      printf("%d", j);
    }
    else {
      printf("-");
    }
  }


  // What the philosopher is doing (it's status)
  int stat = philosophers[i];
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
      break;
  }

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

