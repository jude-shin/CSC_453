#include <stdio.h>
#include <stdlib.h>
#include "table.h"

// you want six semaphores
void print_break_line(int col_width, int full_line_width);
void print_name_line(Phil *head, int col_width, int full_line_width);
void print_status(Phil *curr, int col_width, int full_line_width);

// Prints a line of "=" with occasional "|" for the number of philosophers
void print_break_line(int col_width, int full_line_width) {
  printf("|");
  for (int i=0; i<NUM_PHILOSOPHERS; i++) {
    for (int j=0; j<col_width; j++) {
      printf("=");
    }
    printf("|");
  }
  printf("\n");
}

void print_name_line(Phil *head, int col_width, int full_line_width) {
  Phil *curr = head;
  int padding = (col_width-1)/2;

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
void print_status_line(Phil *head, int col_width, int full_line_width) {
  Phil *curr = head;
  int middle = (col_width+1)/2;

  printf("|");
  for (int i=0; i<NUM_PHILOSOPHERS; i++) {
    print_status(curr, col_width, full_line_width);
    curr = curr->right->right;
  }
  printf("\n");
}

// prints just one status
void print_status(Phil *curr, int col_width, int full_line_width) {
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
      printf(stderr, "error parding curr->doing. unknown: %s", msg);
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



