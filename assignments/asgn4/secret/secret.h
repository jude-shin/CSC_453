#ifndef __SECRET_H
#define __SECRET_H

/* The Hello, World! message. */
#define HELLO_OG "Hello, World! (I'm the OG)\n"
#define HELLO_IN "Hello writing into driver!\n"

/* Fixed size of the secret to be stored */
#ifndef SECRET_SIZE
#define SECRET_SIZE 8192
#endif /* SECRET_SIZE */

/* For print statements */
#ifndef DEBUG 
#define DEBUG 0 
#endif /* DEBUG  */

#endif /* __SECRET_H */
