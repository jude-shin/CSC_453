#ifndef TABLE 
#define TABLE 

// How many philosophers will be fighting to the death for their spagetti
// NOTE: there will be an equal number of forks as there are philosophers
#ifndef NUM_PHILOSOPHERS
#define NUM_PHILOSOPHERS 6
#endif 

// What the first thread should be labeled as
#ifndef START_CHAR 
#define START_CHAR 'A'
#endif 

// ===========================================================================

// What the philosopher IS DOING
#define CHANGING 0
#define EATING   1
#define THINKING 2

// For printing
// NOTE: you must make all of these the same size so the printing looks nice
#define CHNG_MSG "     "
#define EAT_MSG  "Eat  "
#define THNK_MSG "Think"

#include <semaphore.h>

// Runs through the lifecycle of a single philosopher, eating and thinking.
void *dine(void *p);
void *test_dine(void *ip);

// Get the label based on the index (id) of a Phil. (0->'A', 1->'B' ...)
char get_label(int id);

// Mallocs and sets up the pointers for all forks and philosophers in 
// sequential order
void set_table(void);
void clean_table(void);

#endif 
