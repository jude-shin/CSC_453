#ifndef TABLE 
#define TABLE 

#ifndef NUM_PHILOSOPHERS
#define NUM_PHILOSOPHERS 2
#endif 

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// 1 + (#of phils) + 1 + (len longest message) + 1
// #define COL_WIDTH 1+NUM_PHILOSOPHERS+1+CHNG_MSG+1
// #define CHAR_IN_FULL_LINE 1+(COL_WIDTH+1)*NUM_PHILOSOPHERS

// ===========================================================================

// What the philosopher IS DOING
#define THINKING 0
#define EATING   1
#define CHANGING 2

// What the philosopher WANTS
// These are different from what the IS DOING so I don't royally mess up
#define THINKY 3
#define HUNGRY 4

// For printing
// NOTE: you must make all of these the same size so the printing looks nice
#define CHNG_MSG "     "
#define EAT_MSG  " Eat "
#define THNK_MSG "Think"

#endif 

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
