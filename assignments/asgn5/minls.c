#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>

/* Verbosity [0-2]. The higher the number, the more this program talks. */
int verbosity  = 0;

int main (int argc, char *argv[]) {
  int opt, nsecs;

  while ((opt = getopt(argc, argv, "vp:s")) != -1) {
    switch (opt) {
      case 'v':
        verbosity++;
        break;
      case 'p':
        printf("we got a t\n");
        printf("optarg: %d\n", atoi(optarg));
        printf("argument index: %s\n", argv[optind]);
        break;
      default:
        fprintf(stderr, "Usage: %s [-v] [-p part [-s subpart]] imagefile [path]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }
  int remainder = argc - optind;

  if (remainder == 0) {
    // There are too little arguments
    fprintf(stderr, "Too few arguments.\n");
  }

  if (remainder == 1) {
    char* imagefile = argv[optind++];
    printf("imagefile: %s\n", imagefile);
  }
  if (remainder == 2) {
    char* path = argv[optind++];
    printf("path: %s\n", path);
  }

  if (remainder >=3) {
    fprintf(stderr, "Too many arguments.\n");
    fprintf(stderr, "Usage: %s [-v] [-p part [-s subpart]] imagefile [path]\n", argv[0]);
  }

  exit(EXIT_SUCCESS);
}

