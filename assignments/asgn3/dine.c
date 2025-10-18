#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "dine.h"
#include "phil.h"

// How many philosophers will be fighting to the death for their spagetti
// NOTE: there will be an equal number of forks as there are philosophers
#ifndef NUM_PHILOSOPHERS
#define NUM_PHILOSOPHERS 5
#endif 

void set_seed() {
  struct timeval tp;
  if (gettimeofday(&tp, NULL) == -1) {
    perror("[set_seed] error getting time of the day");
    exit(EXIT_FAILURE);
  }
  srandom(tp.tv_sec + tp.tv_usec);
}



int main (int argc, char *argv[]) {
  // How many times each philosopher should go though their eat-think lifecycle
  // before exiting.
  int lifetime = 3; 

  // The only (optional) command line argument is to change the lifecycle of a 
  // philosopher.
  int err = 0;
  for (int i=2; i<argc; i++) {
    fprintf(stderr,"%s: too many args!\n",argv[i]);
    err++;
  }
  if (argc == 2) {
    // TODO: somehow check to see that this really is an integer
    int l = atoi(argv[1]);
    if (l == 0) {
      perror("[main] error parsing lifetime argument.");
      err++;
    }
    lifetime = l;
  }
  if (err) {
    fprintf(stderr,"usage: %s [n]\n",argv[0]);
    fprintf(stderr, "n: lifecycle of a philosopher.\n");
    exit(err);
  }

  // ------------------------------------------------------------------------ 

  set_seed();

  // Do whatever you want now
  // sem_overview

  // printf("lifetime: %d", lifetime);


}
