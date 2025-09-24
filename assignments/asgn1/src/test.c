#include <stdio.h>
#include "alloc.h"
#include "datastructures.h"
#include "unk.h"

int main(int argc, char *argv[]) {
  my_malloc(sizeof("HELLO WORLD!123456789012345"));

  Chunk *head = get_head();
  

  return 0;
}

