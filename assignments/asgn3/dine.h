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

// All of the forks
extern sem_t forks[NUM_PHILOSOPHERS];

// The semaphore for printing
extern sem_t print;

#endif
