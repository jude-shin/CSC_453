#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  char *original_string = "Hello, World!";
  char *duplicated_string = strdup(original_string);

  if (duplicated_string != NULL) {
    free(duplicated_string);
  }

  return 0;
}

