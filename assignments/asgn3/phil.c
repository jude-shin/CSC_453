#include <stdio.h>
#include <stdlib.h>
#include "phil.h"


// Prints the status for a given philosopher
// Usage: you should probably make all of the changes to the philosopher, then
// print the status on one line, and then switch statuses
void print_status(Phil p) {
  // A header with all of the 
  // There will be an inital [pipe] on the beginning of every line.
  // Then, for every philosopher, they print 1 [space], 5 [dashes], 1 [space], 
  // their status (5 spaces are allocated for the status; "EAT  ", or "THINK", 
  // 1 [space], and finally, 1 [pipe]. An example is shown below:
  /* ----- 12345 |*/
  printf("");
  if (2) {
    perror("[print_status] error doing something");
    exit(EXIT_FAILURE);
  }

}

void print_name_lines() {

}


// Get label for the philosopher based on an i
char get_label(int i) {
  // Start at ascii character 'A'
  char c = 'A';

  // Increment the ascii value a number of times
  return c + i;
}

