#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "table.h"


void print_break_lines();
void print_name_lines(Phil *head);
void print_status_lines(Phil *head);
char get_label(int id);


// TODO: ask him if this is good or if I should make a macro
// TODO: errorcheck strlen
// 1 for the leftmost padding
// n for the number of philosophers // TODO: or is it forks?
// 1 for dividing padding
// msg_len for the length of it's status 
// 1 for the rightmost padding
static int col_width = 1+NUM_PHILOSOPHERS+1+(strlen(CHNG_MSG))+1;

// // 1 for the leftmost "wall"
// // for each of the philosophers: add the col width + 1 for the rightmost edge
// static int full_line_width = 1+(col_width+1)*NUM_PHILOSOPHERS;


// Prints the status line for ALL philosophers 
void print_status(Phil *head) {
  print_break_lines();
  print_name_lines(head);
  print_break_lines();

  // print all of the statuses here

  print_break_lines();
  printf("ALL FINISHED WITH HEADER\n");
}

// Prints a line of "=" with occasional "|" for the number of philosophers
void print_break_lines() {
  int full_line_width = 1+(col_width+1)*NUM_PHILOSOPHERS;
  char l[full_line_width];
  for (int i=0; i<full_line_width; i++) {
    l[i] = '=';

    if (i%col_width+1 == 1) {
      l[i] = '|';
    }
  }

  printf("%s\n", l);
}

void print_name_lines(Phil *head) {
  int full_line_width = 1+(col_width+1)*NUM_PHILOSOPHERS;
  char l[full_line_width];
  int middle = col_width/2;

  l[0] = '|';

  int i = 0;
  Phil *curr = head->right->right;
  while (curr != head) {
    char label = get_label(curr->id);
    for (int j=0; j<col_width; j++) {
      l[i+j] = ' ';
      if ((i+j) == middle) {
        l[i+j] = label;
      }
    }

    curr = curr->right->right;
  }

  printf("%s\n", l);
}


void print_status_lines(Phil *head) {

}

// Get label for the philosopher based on an i
char get_label(int id) {
  // Start at ascii character 'A'
  char c = 'A';

  // Increment the ascii value a number of times
  return c + id;
}

