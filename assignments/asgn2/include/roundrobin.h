#ifndef ROUNDROBIN 
#define ROUNDROBIN 

#include <lwp.h>
extern scheduler MyRoundRobin; 

extern void rr_init(void);
extern void rr_shutdown(void);
extern void rr_admit(thread new);
extern void rr_remove(thread victim);
extern thread rr_next(void);
extern int rr_qlen(void);
#endif
