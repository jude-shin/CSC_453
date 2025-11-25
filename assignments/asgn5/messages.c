#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "messages.h"

/* MINLS */
/* Prints an error that shows the flags that can be used with minls.
 * @param void. 
 * @return void.
 */
void minls_usage(void) {
  fprintf(
      stderr,
      "usage: minls [-v] [-p num [-s num]] imagefile [path]\n"
      "Options:\n"
      "-p part    --- select partition for filesystem (default: none)\n"
      "-s sub     --- select subpartition for filesystem (default: none)\n"
      "-v verbose --- increase verbosity level\n"
      );
}

/* MINGET */
/* Prints an error that shows the flags that can be used with minls.
 * @param void. 
 * @return void.
 */
void minget_usage(void) {
  fprintf(
      stderr,
      "usage: minget [-v] [-p num [-s num]] imagefile srcpath [dstpath]\n"
      "Options:\n"
      "-p part    --- select partition for filesystem (default: none)\n"
      "-s sub     --- select subpartition for filesystem (default: none)\n"
      "-v verbose --- increase verbosity level\n"
      );
}


/* GENERAL */
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
    minls_usage();
    exit(EXIT_FAILURE);
  } else if (*end != '\0') {
    /* There were some non-numberic characters if the end pointer still points
       to null. */
    fprintf(stderr, "Invalid characters found after the number: '%s'\n", end);
    minls_usage();
    exit(EXIT_FAILURE);
  } else if (errno == ERANGE) {
    /* Checks for overflow (and underflow) of the converted value. */
    fprintf(stderr, "Value out of range for long.\n");
    minls_usage();
    exit(EXIT_FAILURE);
  } 

  return value;
}


