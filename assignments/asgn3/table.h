#ifndef TABLE 
#define TABLE 

// How many philosophers will be fighting to the death for their spagetti
// NOTE: there will be an equal number of forks as there are philosophers
#ifndef NUM_PHILOSOPHERS
#define NUM_PHILOSOPHERS 5
#endif 

// Booleans
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
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


// A Philosopher
typedef struct Phil {
  int id;      // The identifier of the philosopher (converted to ascii later) 
  int doing;   // What the philosopher is doing (thinking, eating, or changing)
  struct Fork *right; // The fork to the right
  struct Fork *left;  // The fork to the left 
} Phil;

// A Fork
typedef struct Fork {
  int id;       // The identifier of the fork
  int in_use;   // If the fork is in use or not (TRUE or FALSE)
  struct Phil *right; // The philosopher to the right
  struct Phil *left;  // The philosopher to the left
} Fork;


void *dine(void *p);
void *test_dine(void *p);
char get_label(int id);

#endif 
