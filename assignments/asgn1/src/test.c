#include <string.h>

#include "alloc.h"

int main(int argc, char *argv[]) {
  char *ptr1 = (char *)my_malloc(128);
  char *ptr2 = (char *)my_malloc(64);
  char *ptr3 = (char *)my_malloc(64);

  my_free(ptr1);


  memcpy(ptr2, "1234567812345678123456781234567812345678123456781234567812345678", 64);

  char *realloced = (char *)my_realloc(ptr1, 16);

  return 0;
}

