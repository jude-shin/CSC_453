#ifndef __SECRET_H
#define __SECRET_H

/* The Hello, World! message. */
#define GATHER_MSG "-- gathering --\n"
#define SCATTER_MSG "-- scattering --\n"

/* Fixed size of the secret to be stored */
#ifndef SECRET_SIZE
#define SECRET_SIZE 8192
/* #define SECRET_SIZE 2 */ 
#endif /* SECRET_SIZE */


#ifndef FALSE 
#define FALSE 1
#endif /* FALSE */

#ifndef TRUE 
#define TRUE 0
#endif /* TRUE */

/* For print statements
   to remove them, just comment the macro definition out */
#define DEBUG 1


#endif /* __SECRET_H */
