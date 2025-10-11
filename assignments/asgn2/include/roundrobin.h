#ifndef ROUNDROBIN 
#define ROUNDROBIN 

#include <lwp.h>
extern scheduler MyRoundRobin;

extern void my_rr_init(void);
extern void my_rr_shutdown(void);
extern void my_rr_admit(thread new);
extern void my_rr_remove(thread victim);
extern thread my_rr_next(void);
extern int my_rr_qlen(void);
#endif
