#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void clean_close(int fd) {
  if (close(fd) == -1) {
    perror("error closing file descriptor");
    exit(EXIT_FAILURE);
  }
}
