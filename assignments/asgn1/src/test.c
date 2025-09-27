#include <string.h>

#include "alloc.h"

int main(int argc, char *argv[]) {
  char *ptr1 = (char *)my_malloc(16);

  memcpy(ptr1, "1234567812345678", 16);

  // char *realloced_ptr = (char *)my_realloc(ptr1, 32); 

  return 0;
}

