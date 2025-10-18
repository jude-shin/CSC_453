CC=/bin/gcc
CFLAGS = -Wall -g -pedantic -fPIC -I ./include
LD = /bin/gcc
LDFLAGS = -Wall -g -pedantic -lrt -lpthreads
DEMO_DIR = ~pn-cs453/demos/

ALL_PROGS = dine

SAMPLE_PROGS = $(DEMO_DIR)dine

TEST_PROGS = $(DEMO_DIR)tryAsgn3~ $(DEMO_DIR)tryAsgn3

.PHONY: all allclean clean 

# =====================================================================

all: $(ALL_PROGS)

allclean: clean
	rm -f $(ALL_PROGS) 

clean:	
	rm -f *.o

test: 


# =====================================================================

dine: dine.o phil.o
	$(LD) $(LDFLAGS) -o $@ $^

dine.o: dine.c
	$(CC) $(CFLAGS) -c $^ -o $@

phil.o: phil.c
	$(CC) $(CFLAGS) -c $^ -o $@

# =====================================================================
