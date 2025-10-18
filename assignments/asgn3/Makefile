CC=/bin/gcc
CFLAGS = -Wall -g -pedantic -fPIC -I ./include
LD = /bin/gcc
LDFLAGS = -Wall -g -pedantic -lrt -lpthread
DEMO_DIR = ~pn-cs453/demos/

ALL_PROGS = dine
SAMPLE_PROGS = $(DEMO_DIR)dine
TEST_PROGS = $(DEMO_DIR)tryAsgn3~ $(DEMO_DIR)tryAsgn3

OBJS = dine.o phil.o dawdle.o

.PHONY: all allclean clean 

# =====================================================================

all: $(ALL_PROGS)

allclean: clean
	rm -f $(ALL_PROGS) 

clean:	
	rm -f $(OBJS)

test: 
	echo notImplementedYet....

# =====================================================================

dine: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@


# =====================================================================
