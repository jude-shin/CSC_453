#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>

#include "minls.h"

/* Verbosity [0-2]. The higher the number, the more this program talks. */
int verbosity  = 0;

int main (int argc, char *argv[]) {
  int opt, nsecs;

  /* Loop through all of the arguments, only accepting the flagw -v -p and -s
   * Where -p and -s have arguments after it.
   */
  while ((opt = getopt(argc, argv, "vp:s:")) != -1) {
    switch (opt) {
      /* Verbosity flag */
      case 'v':
        verbosity++;
        break;
      /* Partition flag (main) */
      case 'p':
        printf("we got a (main) partition\n");
        printf("optarg: %d\n", atoi(optarg));
        printf("argument index: %s\n", argv[optind]);
        break;
      /* Partition flag (sub) */
      case 's':
        printf("we got a (sub) partition\n");
        printf("optarg: %d\n", atoi(optarg));
        printf("argument index: %s\n", argv[optind]);
        break;
      /* If there is an unknown flag. */
      default:
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
  }
  int remainder = argc - optind;

  /* There aren't enough arguments. */
  if (remainder == 0) {
    fprintf(stderr, "Too few arguments.\n");
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }
  
  /* There is an imagefile. */
  if (remainder >= 1) {
    char* imagefile = argv[optind++];
    printf("imagefile: %s\n", imagefile);
  }

  /* There is also a path. */
  if (remainder == 2) {
    char* path = argv[optind++];
    printf("path: %s\n", path);
  }

  /* There are too many arguments. */
  if (remainder >= 3) {
    fprintf(stderr, "Too many arguments.\n");
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}

/* Prints an error that shows the flags that can be used.
 * @param command The name of the command that is trying to be run (minls).
 * @return void.
 */
void print_usage(char* command) {
  fprintf(stderr, "Usage: %s [-v] [-p part [-s subpart]] imagefile [path]\n", command);
}

