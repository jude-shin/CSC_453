#include <stdio.h>
#include "alloc.h"

int main(int argc, char *argv[]) {
  my_malloc(sizeof("HELLO WORLD!"));

  my_malloc(sizeof("HELLO WORLD!"));

  my_malloc(sizeof("HELLO WORLD!"));

  my_malloc(sizeof("HELLO WORLD!"));

  return 0;
}

