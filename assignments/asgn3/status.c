#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "table.h"
#include "status.h"

// TODO: test that all of the strings that are

// you want six semaphores

static int col_width = 0;

// Prints a line of "=" with occasional "|" for the number of philosophers
void print_break_line() {
  printf("|");
  for (int i=0; i<NUM_PHILOSOPHERS; i++) {
    for (int j=0; j<get_col_width(); j++) {
      printf("=");
    }
    printf("|");
  }
  printf("\n");
}

void print_name_line(Phil *head) {
  Phil *curr = head;
  int padding = (get_col_width()-1)/2;

  printf("|");

  for (int i=0; i<NUM_PHILOSOPHERS; i++) {
    char label = get_label(curr->id);
    for (int j=0; j<padding; j++) {
      printf(" ");
    }

    printf("%c", label);

    for (int j=0; j<padding; j++) {
      printf(" ");
    }
    printf("|");
    curr = curr->right->right;
  }
  printf("\n");
}

// Prints ALL the statuses 
void print_status_line(Phil *head) {
  Phil *curr = head;
  printf("|");
  for (int i=0; i<NUM_PHILOSOPHERS; i++) {
    print_status(curr);
    curr = curr->right->right;
  }
  printf("\n");
}

// prints just one status
void print_status(Phil *curr) {
  const char *msg = "";
  switch (curr->doing) {
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

  printf(" ");
  
  Fork *left = curr->left;
  Fork *right = curr->right;
  for (int i=0; i<NUM_PHILOSOPHERS; i++) {
    if (i == left->id && left->in_use == TRUE) {
      printf("%d", i);
    }
    else if (i == right->id && right->in_use == TRUE) {
      printf("%d", i);
    }
    else {
      printf("-");
    }
  }

  printf(" %s", msg);
  printf(" |");
}

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

