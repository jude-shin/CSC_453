#include<string.h>
#include<stdlib.h>
#include<stdio.h>

int main(int argc, char *argv[]) {
  // void *s = NULL;
  // s = malloc(7);

  char *s = NULL;
  s = strdup("Try Me");
  puts(s);
  free(s);
  return 0;
}

