#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>

#include "minls.h"
#include "messages.h"

int main (int argc, char *argv[]) {
  /* Verbosity [0-2].
     The higher the number, the more this program talks. */
  int verbosity = 0;
  
  /* What partition number to use [0-3].
     (-1 means unpartitioned. )*/
  long partition = -1;

  /* What subpartition number to use [0-3].
     (-1 means unpartitioned. ) */
  long subpartition = -1;


  /* Loop through all of the arguments, only accepting the flagw -v -p and -s
     Where -p and -s have arguments after it. */
  int opt;
  while ((opt = getopt(argc, argv, "vp:s:")) != -1) {
    switch (opt) {
      /* Verbosity flag */
      case 'v':
        verbosity++;
        break;

      /* Partition flag (primary) */
      case 'p':
        partition = parse_int(optarg);
        printf("we got a (primary) partition: %ld\n", partition);
        /* TODO: check that the there are only 4 partiitons (>=3) */
        if (partition < 0) {
          fprintf(stderr, "The partition must be non-negative.\n");
          minls_usage();
          exit(EXIT_FAILURE);
        }
        break;

      /* Partition flag (sub) */
      case 's':
        subpartition = parse_int(optarg);
        printf("we got a (sub) partition: %ld\n", subpartition);
        /* TODO: check that the there are only 4 partiitons (>=3) */
        if (subpartition < 0) {
          fprintf(stderr, "The subpartition must be non-negative.\n");
          minls_usage();
          exit(EXIT_FAILURE);
        }
        break;

      /* If there is an unknown flag. */
      default:
        minls_usage();
        exit(EXIT_FAILURE);
    }
  }
  
  /* If the subpartition is set, the primary partition must also be set. */
  if (subpartition != -1 && partition == -1) {
    fprintf(stderr, "A primary partition must also be selected.\n");
    minls_usage();
    exit(EXIT_FAILURE);
  }

  /* How many arguments are there (aside from the flags). */
  int remainder = argc - optind;

  /* There aren't enough arguments. */
  if (remainder == 0) {
    fprintf(stderr, "Too few arguments.\n");
    minls_usage();
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
    minls_usage();
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}

