#ifndef DAWDLE 
#define DAWDLE

/* Sets seed for the PRNG that is used in dawdle 
   NOTE: this will also seed anyone else that uses the random(3). */
void set_seed(void);

/* Wait for a random amount of time. */ 
void dawdle(void);

#endif
