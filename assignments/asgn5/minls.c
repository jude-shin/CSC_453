#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

#include "minls.h"

/* Prints an error that shows the flags that can be used with minls.
 * @param void. 
 * @return void.
 */
void print_usage(void) {
  /* TODO: write more usage*/
  fprintf(stderr, "Usage: minls [-v] [-p num [-s num]] imagefile [path]\n");
}

/* Safely parses an argument that is supposed to be an integer.
 * @param s the string to be converted. 
 * @return the long that was converted.
 */
long parse_int(char* s) {
  long value;
  char* end;
  int errno; /* what kind of error occured. */

  errno = 0;

  value = strtol(s, &end, 10);

  if (end == s) {
    /* If the conversion could not be performed. */
    fprintf(stderr, "No digits were found at the beginning of the argument.\n");
    print_usage();
    exit(EXIT_FAILURE);
  } else if (*end != '\0') {
    /* There were some non-numberic characters if the end pointer still points
       to null. */
    fprintf(stderr, "Invalid characters found after the number: '%s'\n", end);
    print_usage();
    exit(EXIT_FAILURE);
  } else if (errno == ERANGE) {
    /* Checks for overflow (and underflow) of the converted value. */
    fprintf(stderr, "Value out of range for long.\n");
    print_usage();
    exit(EXIT_FAILURE);
  } 

  return value;
}

  int main (int argc, char *argv[]) {
    /* Verbosity [0-2]. The higher the number, the more this program talks. */
  int verbosity = 0;
  
  /* What partition number to use. (-1 means unpartitioned. )*/
  long partition = -1;

  /* What subpartition number to use. (-1 means unpartitioned. ) */
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

      /* Partition flag (main) */
      case 'p':
        partition = parse_int(optarg);
        printf("we got a (main) partition: %ld\n", partition);
        if (partition < 0) {
          fprintf(stderr, "The partition must be non-negative.\n");
          print_usage();
          exit(EXIT_FAILURE);
        }
        break;

      /* Partition flag (sub) */
      case 's':
        subpartition = parse_int(optarg);
        printf("we got a (sub) partition: %ld\n", subpartition);
        if (subpartition < 0) {
          fprintf(stderr, "The subpartition must be non-negative.\n");
          print_usage();
          exit(EXIT_FAILURE);
        }
        break;

      /* If there is an unknown flag. */
      default:
        print_usage();
        exit(EXIT_FAILURE);
    }
  }
  
  /* If the subpartition is set, the partition must also be set. */
  if (subpartition != -1 && partition == -1) {
    fprintf(stderr, "A partition must also be selected.\n");
    print_usage();
    exit(EXIT_FAILURE);
  }

  /* How many arguments are there (aside from the flags). */
  int remainder = argc - optind;

  /* There aren't enough arguments. */
  if (remainder == 0) {
    fprintf(stderr, "Too few arguments.\n");
    print_usage();
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
    print_usage();
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}

