#include <string.h>

#include "alloc.h"

int main(int argc, char *argv[]) {
  char *ptr1 = (char *)my_malloc(16);
  strcpy(ptr1, "1234567812345678");


  char *ptr2 = (char *)my_realloc(ptr1, 16);

  return 0;
}

