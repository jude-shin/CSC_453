#include "alloc.h"

int main(int argc, char *argv[]) {
  void *ptr1 = my_malloc(64);
  void *ptr2 = my_malloc(32);
  void *ptr3 = my_malloc(16);
  // void *ptr4 = my_malloc(16);

  my_free(ptr1);
  my_free(ptr3);

  my_free(ptr2);

  return 0;
}

