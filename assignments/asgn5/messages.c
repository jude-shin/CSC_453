#include <stdio.h>

#include "messages.h"
#include "disk.h"

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
/* Prints all of the information in a partition table. 
 * @param pt a pointer to the partition table struct
 * @return void. 
 */
void print_part_table(part_tbl* pt) {
  printf("\n\n=== PARTITION TABLE ===\n\n");

  printf("bootind: %d\n", pt->bootind);
  printf("start_head: %d\n", pt->start_head);
  printf("start_sec: %d\n", pt->start_sec);
  printf("start_cyl: %d\n", pt->start_cyl);
  printf("type: %d\n", pt->type);
  printf("end_head: %d\n", pt->end_head);
  printf("end_sec: %d\n", pt->end_sec);
  printf("end_cyl: %d\n", pt->end_cyl);
  printf("lFirst: %d\n", pt->lFirst);
  printf("size: %d\n", pt->size);

  printf("\n=== (end partition table) ===\n\n");
}



