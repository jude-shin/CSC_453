



#include "phil.h"


#define CHANGING = 0
#define EATING   = 1
#define THINKING = 2

// Get label for the philosopher based on an i
char get_label(int i) {
  // Start at ascii character 'A'
  char c = 'A';

  // Increment the ascii value a number of times
  return c + i;
}

