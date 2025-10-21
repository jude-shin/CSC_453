#ifndef TABLE 
#define TABLE 

#ifndef NUM_PHILOSOPHERS
#define NUM_PHILOSOPHERS 5
#endif 

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef START_CHAR 
#define START_CHAR 'A'
#endif 

// ===========================================================================

// What the philosopher IS DOING
#define CHANGING 0
#define EATING   1
#define THINKING 2

// What the philosopher WANTS
// These are different from what the IS DOING so I don't royally mess up
#define THINKY 3
#define HUNGRY 4

// For printing
// NOTE: you must make all of these the same size so the printing looks nice
#define CHNG_MSG "     "
#define EAT_MSG  "Eat  "
#define THNK_MSG "Think"


// A Philosopher
typedef struct Phil {
  int id;      // The identifier of the philosopher (converted to ascii later) 
  int doing;   // What the philosopher is doing (thinking, eating, or changing)
  int wants;   // What the philosopher currently wants (thinky, hungry)
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


void dine(void *phil);
char get_label(int id);

#endif 
