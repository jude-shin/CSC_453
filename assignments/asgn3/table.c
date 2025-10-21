#include <stdio.h>
#include "table.h"


// you want six semaphores

// Prints the status line for ALL philosophers 
void print_status(Phil *head, int col_width, int full_line_width) {
  print_break_lines(col_width, full_line_width);
  print_name_lines(head, col_width, full_line_width);
  print_break_lines(col_width, full_line_width);

  // print all of the statuses here

  print_break_lines(col_width, full_line_width);
}

// Prints a line of "=" with occasional "|" for the number of philosophers
void print_break_lines(int col_width, int full_line_width) {
  char l[full_line_width+1];
  l[full_line_width] = '\0';

  for (int i=0; i<full_line_width; i++) {
    l[i] = '=';

    if (i%(col_width+1) == 0) {
      l[i] = '|';
    }
  }

  printf("%s\n", l);
}

void print_name_lines(Phil *head, int col_width, int full_line_width) {
  char l[full_line_width+1];
  l[full_line_width] = '\0';

  Phil *curr = head;
  int middle = (col_width+1)/2;

  for (int i=0; i<full_line_width; i++) {
    l[i] = ' ';

    if (i%(col_width+1) == 0) {
      l[i] = '|';
    }
    else if (i%middle == 0) {
      char label = get_label(curr->id);
      curr = curr->right->right;
      l[i] = label;
    }
  }

  printf("%s\n", l);
}


void print_status_lines(Phil *head, int col_width, int full_line_width) {

}

// Get label for the philosopher based on an i
char get_label(int id) {
  // Start at ascii character 'A'
  char c = 'A';

  // Increment the ascii value a number of times
  return c + id;
}

