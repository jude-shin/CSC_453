#include <limits.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef DAWDLEFACTOR
#define DAWDLEFACTOR 1000
#endif
void dawdle() {
  /*
   * sleep for a random amount of time between 0 and DAWDLEFACTOR
   * milliseconds. This routine is somewhat unreliable, since it
   * doesn’t take into account the possiblity that the nanosleep
   * could be interrupted for some legitimate reason.
   */

  // TODO: ask if we need to make a different function, or if this "unreliable"
  // routine is good enough
  struct timespec tv;
  int msec = (int)((((double)random()) / RAND_MAX) * DAWDLEFACTOR);
  tv.tv_sec = 0;
  tv.tv_nsec = 1000000 * msec;
  if ( -1 == nanosleep(&tv,NULL) ) {
    perror("nanosleep");
    exit(EXIT_FAILURE);
  }
}
