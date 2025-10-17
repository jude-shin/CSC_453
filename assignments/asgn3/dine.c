#include <stdlib.h>
#include <stdio.h>

#include "dine.h"
// #include "phil.h"

// How many philosophers will e 
#ifndef NUM_PHILOSOPHERS
#define NUM_PHILOSOPHERS 5
#endif 




int main (int argc, char *argv[]) {
  // How many times each philosopher should go though their eat-think lifecycle
  // before exiting.
  int lifetime = 1; 
 

  // The only (optional) command line argument is to change the lifecycle of a 
  // philosopher.
  int err = 0;
  for (int i=2; i<argc; i++) {
    fprintf(stderr,"%s: unknown option\n",argv[i]);
    err++;
  }
  if (argc == 2) {
    int l = atoi(argv[2]);
    if (l == 0) {
      perror("[main] error parsing lifetime argument.");
      err++;
    }
  }
  if (err) {
    fprintf(stderr,"usage: %s [options]\n",argv[0]);
    fprintf(stderr, "n: lifecycle of a philosopher.\n");
    exit(err);
  }

  // Do whatever you want now
}
