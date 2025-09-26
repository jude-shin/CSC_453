#include <string.h>

#include "alloc.h"

int main(int argc, char *argv[]) {
  char *ptr1 = (char *)my_malloc(16);
  char *ptr2 = (char *)my_malloc(16);
  char *ptr3 = (char *)my_malloc(15);


  my_free(ptr2);

  char *ptr4 = (char *)my_malloc(16); // should go into the second slot

  my_free(ptr1);
  my_free(ptr3);

  my_free(ptr4); // should merge with everything and become just one head

  // memcpy(ptr2, "1234567812345678123456781234567812345678123456781234567812345678", 64);

  return 0;
}

