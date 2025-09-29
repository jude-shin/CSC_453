#include<string.h>
#include<stdlib.h>
#include<stdio.h>

int main(int argc, char *argv[]) {

  char *line = NULL;
  size_t len = 0;
  printf("123456789\n"); 

  getline(&line, &len, stdin);
  free(line);


  return 0;
}

