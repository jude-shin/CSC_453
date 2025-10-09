CC=/bin/gcc
CFLAGS = -Wall -g -fPIC -I ./include
LD = /bin/gcc
LDFLAGS = -Wall -g -L lib64

TEST_LWP_LIB = lib64/libPLN.so 
# LWP_LIB = liblwp.so
LWP_LIB = $(TEST_LWP_LIB)

TEST_PROGS	= snakes nums hungry
SNAKE_OBJS = bin/randomsnakes.o bin/util.o
SNAKE_LIBS = -lPLN -lsnakes -lncurses -lrt
HUNGRY_OBJS = bin/hungrysnakes.o bin/util.o
NUM_OBJS = bin/numbersmain.o
TEST_OBJS = $(SNAKE_OBJS) $(HUNGRY_OBJS) $(NUM_OBJS) 

EXTRA_CLEAN = core $(TEST_PROGS)

.PHONY: all allclean clean libmalloc.so

all: $(TEST_PROGS)

allclean: clean
	@rm -f $(EXTRACLEAN)

clean:	
	rm -f $(TEST_OBJS) *~ TAGS

snakes: bin/randomsnakes.o bin/util.o $(LWP_LIB) lib64/libsnakes.so
	$(LD) $(LDFLAGS) -o snakes bin/randomsnakes.o bin/util.o $(SNAKE_LIBS)

hungry: bin/hungrysnakes.o bin/util.o $(LWP_LIB) lib64/libsnakes.so
	$(LD) $(LDFLAGS) -o hungry bin/hungrysnakes.o bin/util.o $(SNAKE_LIBS)

nums: bin/numbersmain.o bin/util.o $(LWP_LIB) 
	$(LD) $(LDFLAGS) -o nums bin/numbersmain.o -lPLN

bin/hungrysnakes.o: demos/hungrysnakes.c include/lwp.h include/snakes.h
	$(CC) $(CFLAGS) -c demos/hungrysnakes.c -o $@

bin/randomsnakes.o: demos/randomsnakes.c include/lwp.h include/snakes.h
	$(CC) $(CFLAGS) -c demos/randomsnakes.c -o $@

bin/numbersmain.o: demos/numbersmain.c include/lwp.h
	$(CC) $(CFLAGS) -c demos/numbersmain.c -o $@

bin/util.o: demos/util.c include/lwp.h include/util.h include/snakes.h
	$(CC) $(CFLAGS) -c demos/util.c -o $@

libmalloc.so: bin/lwp.o
	$(CC) $(CFLAGS) -shared -o $@ bin/lwp.o 

bin/lwp.o: src/lwp.c
	$(CC) $(CFLAGS) -c $< -o $@
