#include "table.h"

void dine(void *phil) {
  Phil *curr = (Phil*)phil;

}

// Get label for the philosopher based on an i
char get_label(int id) {
  // Start at ascii character 'A'
  char c = START_CHAR;

  // Increment the ascii value a number of times
  return c + id;
}

