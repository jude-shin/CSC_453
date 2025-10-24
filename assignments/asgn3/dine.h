#ifndef DINE 
#define DINE 

#include <semaphore.h>
#include "table.h"

/* The number of cycles that a philosopher will eat/think before dying. */
extern int lifetime;

/* The width of each column. This is useful for printing. */
extern int col_width;

/* All the philosophers (the only information we need to store is what
   state they are in). */
extern Phil philosophers[NUM_PHILOSOPHERS];

/* The index of the philosopher who owns the fork -1 for available. */
extern int forks[NUM_PHILOSOPHERS];

/* Avaliability of forks. Each element in the list will represent who 
   is holding/using it. -1 means that it is free. */
extern sem_t fork_sems[NUM_PHILOSOPHERS];

/* Semaphore for printing. Only one thread can print the status at a time. */
extern sem_t print_sem;

#endif
