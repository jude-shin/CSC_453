#ifndef DINE 
#define DINE 

#include "table.h"
#include <semaphore.h>

// GLOBAL VARIABLES
// The number of cycles that a philosopher will eat/think before dying 
extern int lifetime;

// All the philosophers (the only information we need to store is what
// state they are in)
extern int philosophers[NUM_PHILOSOPHERS];

extern int phil_i[NUM_PHILOSOPHERS];

// Avaliability of forks 
extern sem_t fork_sems[NUM_PHILOSOPHERS];

// The index of the philosopher who owns the fork
// -1 for available
extern int forks[NUM_PHILOSOPHERS];

// The semaphore for printing
extern sem_t print_sem;

#endif
