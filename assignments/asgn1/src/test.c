#include <string.h>

#include "alloc.h"

int main(int argc, char *argv[]) {
  // char *ptr1 = (char *)my_malloc(16);
  // char *ptr2 = (char *)my_malloc(32);
  // char *ptr3 = (char *)my_malloc(64);

  char *ptr1 = (char *)my_malloc(128);
  char *ptr2 = (char *)my_malloc(16);
  char *ptr3 = (char *)my_malloc(15);

  my_free(ptr1);

  memcpy(ptr2, "1234567812345678", 16);

  char *realloced_ptr = (char *)my_realloc(ptr2, 16); 

  return 0;
}

