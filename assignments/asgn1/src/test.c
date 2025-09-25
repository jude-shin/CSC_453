#include "alloc.h"

int main(int argc, char *argv[]) {
  // break here: get the inital size of the global header

  void *ptr = my_malloc(16);

  // break here: get the size of the header

  my_free(ptr);

  // break here: get the size of the header
  // should be the same as the first time you looked

  return 0;
}

