#include<string.h>
#include<stdlib.h>
#include<stdio.h>

int main(int argc, char *argv[]) {
  void *one = malloc(64);
  one = realloc(one, 128);

  void *two = malloc(64); 
  two = realloc(two, 128);

  void *three = malloc(64);
  three = realloc(three, 128);


  free(one);
  free(two);
  free(three);
  return 0;
}

