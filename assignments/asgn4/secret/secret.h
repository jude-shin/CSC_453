#ifndef __SECRET_H
#define __SECRET_H

/*#ifndef FALSE 
#define FALSE 1
#endif 

#ifndef TRUE 
#define TRUE 0
#endif*/

/* For debugging. To disable debugging, comment the macro definition out */
/* #define DEBUG 1 */

/* ===== */
/* MAGIC */
/* ===== */
/* Fixed size of the secret to be stored */
#ifndef SECRET_SIZE
#define SECRET_SIZE 8192
#endif /* SECRET_SIZE */

/* Name of the Driver */
#ifndef DRIVER_NAME 
#define DRIVER_NAME "secretkeeper"
#endif /* DRIVER_NAME */

#endif /* __SECRET_H */
