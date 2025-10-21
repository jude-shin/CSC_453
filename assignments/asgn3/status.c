#include <stdio.h>
#include "table.h"
#include "status.h"

// you want six semaphores
void print_status(Phil *curr, int col_width, int full_line_width);
void print_break_line(int col_width, int full_line_width);
void print_name_line(Phil *head, int col_width, int full_line_width);
void print_status_lines(Phil *head, int col_width, int full_line_width);


// Prints the status line for ALL philosophers 
void print_test(Phil *head, int col_width, int full_line_width) {
  print_break_line(col_width, full_line_width);
  print_name_line(head, col_width, full_line_width);
  print_break_line(col_width, full_line_width);

  print_status_lines(head, col_width, full_line_width);
  print_status_lines(head, col_width, full_line_width);
  print_status_lines(head, col_width, full_line_width);
  print_status_lines(head, col_width, full_line_width);
  print_status_lines(head, col_width, full_line_width);
  print_status_lines(head, col_width, full_line_width);

  print_break_line(col_width, full_line_width);
}

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

// prints just one status
void print_status(Phil *curr, int col_width, int full_line_width) {
  const char *msg = "";
  switch (curr->doing) {
    case CHANGING:
      // msg = CHNG_MSG;
      msg = "chang";
      break;
    case EATING:
      // msg = EAT_MSG;
      msg = "eatys";
      break;
    case THINKING:
      // msg = THNK_MSG;
      msg = "think";
      break;
    default:
      // TODO: do something interesting
      perror("hello");
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

// Prints ALL the statuses 
void print_status_lines(Phil *head, int col_width, int full_line_width) {
  Phil *curr = head;
  int middle = (col_width+1)/2;

  printf("|");
  for (int i=0; i<NUM_PHILOSOPHERS; i++) {
    print_status(curr, col_width, full_line_width);
    curr = curr->right->right;
  }
  printf("\n");
}


