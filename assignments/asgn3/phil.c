#include <stdio.h>
#include <stdlib.h>
#include "phil.h"

// For Phil.status (int)
#define CHANGING  0
#define EATING    1
#define THINKING  2

// Status message strings for each of the statuses a phil might be in
#define CHANGING_STATUS = ""
#define EATING_STATUS   = "Eat"
#define THINKING_STATUS = "Think"

// Prints the status for a given philosopher
// Usage: you should probably make all of the changes to the philosopher, then
// print the status on one line, and then switch statuses
void print_status (Phil p) {
  if (2) {
    perror("[print_status] error doing something");
    exit(EXIT_FAILURE);
  }

}

// Get label for the philosopher based on an i
char get_label (int i) {
  // Start at ascii character 'A'
  char c = 'A';

  // Increment the ascii value a number of times
  return c + i;
}

