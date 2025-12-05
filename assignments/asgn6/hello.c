#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  /* Greet the world by printing "Hello, world!" followed by a newline. */
  /* If this syscall fails, it returns a value less than 0. */
  if (printf("Hello, world!\n") < 0) {
    perror("error with printf()\n");
    exit(-1);
  }

  /* Exit with a zero exit status indicating success. */
  exit(0);
}
