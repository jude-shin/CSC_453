#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#ifndef DAWDLEFACTOR
#define DAWDLEFACTOR 1000
#endif


void set_seed(void) {
  /* Sets the seed to be the number or seconds since the epoch plus the number
     of microseconds since the last second. */
  struct timeval tp;

  if (gettimeofday(&tp, NULL) == -1) {
    fprintf(stderr, "[set_seed] error getting time of the day");
    exit(EXIT_FAILURE);
  }
  srandom(tp.tv_sec + tp.tv_usec);
}

void dawdle(void) {
  /*
   * sleep for a random amount of time between 0 and DAWDLEFACTOR
   * milliseconds. This routine is somewhat unreliable, since it
   * doesn’t take into account the possiblity that the nanosleep
   * could be interrupted for some legitimate reason.
   */
  struct timespec tv;
  int msec = (int)((((double)random()) / RAND_MAX) * DAWDLEFACTOR);
  tv.tv_sec = 0;
  tv.tv_nsec = 1000000 * msec;
  if ( -1 == nanosleep(&tv,NULL) ) {
    perror("nanosleep");
    exit(EXIT_FAILURE);
  }
}
