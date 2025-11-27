#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "input.h"

#define MAX_PRIM_PART 4

/* Parses the flags given. If an error occurs, this function returns -1 and is 
 * handled in the caller.
 * @param argc number of arguments passed to the main function
 * @param argv[] array of strings passes as arguments to the main function
 * @param verbosity a ptr to the int tracking the verbosity
 * @param prim_part a ptr to the int tracking the number of primary partition
 * @param sub_part a ptr to the int tracking the number of sub partitions 
 * @param the 
 * @return the number of flags that were processed. -1 if any error occured.
 */
int parse_flags(
    int argc, 
    char* argv[], 
    int* verbosity, 
    int* prim_part, 
    int* sub_part) {
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
        *prim_part = parse_positive_int(optarg);
        if (*prim_part < 0) {
          fprintf(stderr, "An error occured parsing the primary partition\n");
          return -1;
        }
        if (*prim_part >= MAX_PRIM_PART) {
          fprintf(
              stderr, 
              "The primary partition must be less than %d.\n",
              MAX_PRIM_PART);
          return -1;
        }
        break;

        /* Partition flag (sub) */
      case 's':
        *sub_part = parse_positive_int(optarg);
        if (*sub_part < 0) {
          fprintf(stderr, "An error occured parsing the sub partition\n");
          return -1;
        }
        break;

      /* If there is an unknown flag. */
      default:
        return -1;
    }
  }

  /* If the sub_part is set, the primary partition must also be set. */
  if (*sub_part != -1 && *prim_part == -1) {
    fprintf(stderr, "A primary partition must also be selected.\n");
    return -1;
  }

  return optind;
}

/* Safely parses an argument that is supposed to be an integer. Upon any error 
 * (or if the parsed value is <0) -1 is returned and handled in the caller.
 * @param s the string to be converted. 
 * @return the positive integer that was converted (-1 if any errors were
 *  encountered).
 */
int parse_positive_int(char* s) {
  int value;
  char* end;
  int errno; /* what kind of error occured. */

  errno = 0;

  /* TODO: check for the overflow before casting? */
  value = (int) strtol(s, &end, 10);

  if (end == s) {
    /* If the conversion could not be performed. */
    fprintf(stderr, "No digits were found at the beginning of the argument.\n");
    value = -1;
  }
  else if (*end != '\0') {
    /* There were some non-numberic characters if the end pointer still points
       to null. */
    fprintf(stderr, "Invalid characters found after the number: '%s'\n", end);
    value = -1;
  }
  else if (errno == ERANGE) {
    /* Checks for overflow (and underflow) of the converted value. */
    fprintf(stderr, "Value out of range for long.\n");
    value = -1;
  } 
  else if (value < 0) {
    /* Checks for overflow (and underflow) of the converted value. */
    fprintf(stderr, "The integer must be positive.\n");
    value = -1;
  }

  return value;
}

/* Parses the rest of the input for minls, setting the caller values.  
 * @param argc number of arguments passed to the main function
 * @param argv[] array of strings passes as arguments to the main function
 * @param imagefile a ptr to the string that represents the imagefile (req)
 * @param path a ptr to the string that represents the imagefile. Note that this
 *  is set to the root path "/" if the user does not specify it.
 * @return the number of arguments that were processed, -1 if anything goes
 *  wrong.
 */
int parse_minls_input(
    int argc, 
    char* argv[], 
    char** imagefile,
    char** path) {

  /* The number of flags that were parsed. */
  int i = optind;

  /* How many arguments are there (aside from the flags). */
  int remainder = argc - i;

  /* path is optional, and is set to the default root path. */
  /* size of the path string including the null terminator. */
  size_t path_size = sizeof(char)*strlen("/")+1;
  *path = malloc(path_size);
  if (*path == NULL) {
    fprintf(stderr, "error malloc()'ing path\n");
    return -1;
  }
  memcpy(*path, "/", path_size);

  /* There aren't enough arguments. */
  if (remainder <= 0) {
    fprintf(stderr, "Too few arguments.\n");
    free(*path);
    return -1;
  }

  /* There is an imagefile. */
  if (remainder >= 1) {
    size_t imagefile_size = sizeof(char)*(strlen(argv[i])+1);
    *imagefile = malloc(imagefile_size);
    if (*path == NULL) {
      fprintf(stderr, "error malloc()'ing imagefile\n");
      return -1;
    }
    memcpy(*imagefile, argv[i], imagefile_size);
    i++;
  }

  /* There is also a path. */
  if (remainder >= 2) {
    free(*path);
    path_size = sizeof(char)*strlen(argv[i])+1;
    *path = malloc(path_size);
    if (*path == NULL) {
      fprintf(stderr, "error malloc()'ing path\n");
      return -1;
    }
    memcpy(*path, argv[i], path_size);
  }

  /* There are too many arguments. */
  if (remainder >= 3) {
    fprintf(stderr, "Too many arguments.\n");
    free(*imagefile);
    free(*path);
    return -1;
  }

  return remainder;
}

/* Parses the rest of the input for minget, setting the caller values.  
 * @param argc number of arguments passed to the main function
 * @param argv[] array of strings passes as arguments to the main function
 * @param imagefile a ptr to the string that represents the imagefile (req)
 * @param srcpath a ptr to the string that represents the source path.
 * @param dstpath a ptr to the string that represents the destination path. Note
 *  that this is set to NULL if the user does not specify it.
 * @return the number of arguments that were processed. -1 if anything goes
 *  wrong.
 */
int parse_minget_input(
    int argc, 
    char* argv[], 
    char** imagefile,
    char** srcpath, 
    char** dstpath) {

  /* The number of flags that were parsed. */
  int i = optind;

  /* How many arguments are there (aside from the flags). */
  int remainder = argc - i;

  /* dstpath is optional, and is set to NULL */
  /* TODO: This is later "translated" to stdout */
  *dstpath = NULL;

  /* There aren't enough arguments. */
  if (remainder <= 1) {
    fprintf(stderr, "Too few arguments.\n");
    return -1;
  }

  /* There is an imagefile and srcpath. */
  if (remainder >= 2) {
    size_t imagefile_size = sizeof(char)*strlen(argv[i])+1;
    *imagefile = malloc(imagefile_size);
    if (*imagefile == NULL) {
      fprintf(stderr, "error malloc()'ing imagefile\n");
      return -1;
    }
    memcpy(*imagefile, argv[i], imagefile_size);
    i++;

    size_t srcpath_size = sizeof(char)*strlen(argv[i])+1;
    *srcpath = malloc(srcpath_size);
    if (*srcpath == NULL) {
      fprintf(stderr, "error malloc()'ing srcpath\n");
      free(*imagefile);
      return -1;
    }
    memcpy(*dstpath, argv[i], srcpath_size);
    i++;
  }

  /* There is also a dstpath. */
  if (remainder >= 3) {
    size_t dstpath_size = sizeof(char)*strlen(argv[i])+1;
    *dstpath = malloc(dstpath_size);
    if (*dstpath == NULL) {
      fprintf(stderr, "error malloc()'ing dstpath\n");
      free(*imagefile);
      free(*srcpath);
      return -1;
    }
    memcpy(*dstpath, argv[i], dstpath_size);
    i++;
  }

  /* There are too many arguments. */
  if (remainder >= 4) {
    fprintf(stderr, "Too many arguments.\n");
    free(imagefile);
    free(srcpath);
    free(dstpath);
    return -1;
  }

  return remainder;
}

