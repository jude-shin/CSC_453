#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>

#include<pp.h>

int main(int argc, char *argv[]) {
  char *s = NULL;
  s = strdup("Try Me");

  pp(stdout, "finished the string dup!!!!!!!");
  sleep(10);
  
  puts(s);
  free(s);
  return 0;
}

