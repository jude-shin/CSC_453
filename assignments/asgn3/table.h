#ifndef TABLE 
#define TABLE 
#include <semaphore.h>

/* How many philosophers will be fighting to the death for their spagetti
   NOTE: there will be an equal number of forks as there are philosophers. */
#ifndef NUM_PHILOSOPHERS
#define NUM_PHILOSOPHERS 5
#endif 

/* What the first thread should be labeled as. */
#ifndef START_CHAR 
#define START_CHAR 'A'
#endif 

/* What the philosopher is doing. */
#define CHANGING 0
#define EATING   1
#define THINKING 2

/* Messages based on what the philosopher is doing.
   NOTE: you must make all of these the same size so the printing looks nice. */
#define CHNG_MSG "     "
#define EAT_MSG  "Eat  "
#define THNK_MSG "Think"

/* The constant which forks will set when they are not bein occupied. */
#ifndef NOBODY 
#define NOBODY -1 
#endif 

/* A philosopher. */
typedef struct Phil {
  /* This is basically the index of the philosopher. It is mainly used for the
     forks, and to check if it is even or odd. */
  int id; 

  /* Whether this guy is eating, thinking or changing. */
  int state; 
} Phil;

/* Runs through the lifecycle of a single philosopher, eating and thinking. */
void *dine(void *p);

/* Get the label based on the index (id) of a Phil. (0->'A', 1->'B' ...). */
char get_label(int id);

/* Sets up global variables and semaphores. */
void set_table(void);

/* Destroys semaphores. */
void clean_table(void);

#endif 
