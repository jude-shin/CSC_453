CC=/bin/gcc
CFLAGS = -Wall -g -fPIC -I ./include
LD = /bin/gcc
LDFLAGS = -Wall -g -L lib64


# My lwp lib
LWP_LIB_FLAG = -llwp
LWP_LIB = lib64/liblwp.so

# # Testing lwp lib
# LWP_LIB_FLAG = -lPLN
# LWP_LIB = lib64/libPLN.so 

TEST_PROGS	= snakes nums hungry
SNAKE_OBJS = bin/randomsnakes.o bin/util.o
SNAKE_LIBS = $(LWP_LIB_FLAG) -lsnakes -lncurses -lrt
HUNGRY_OBJS = bin/hungrysnakes.o bin/util.o
NUM_OBJS = bin/numbersmain.o
TEST_OBJS = $(SNAKE_OBJS) $(HUNGRY_OBJS) $(NUM_OBJS) 

EXTRA_CLEAN = core $(TEST_PROGS)

.PHONY: all allclean clean path $(LWP_LIB)

# =====================================================================

all: $(TEST_PROGS)

allclean: clean
	@rm -f $(EXTRACLEAN)

clean:	
	rm -f $(TEST_OBJS) *~ TAGS

# =====================================================================

snakes: bin/randomsnakes.o bin/util.o $(LWP_LIB) lib64/libsnakes.so
	$(LD) $(LDFLAGS) -o snakes bin/randomsnakes.o bin/util.o $(SNAKE_LIBS)

hungry: bin/hungrysnakes.o bin/util.o $(LWP_LIB) lib64/libsnakes.so
	$(LD) $(LDFLAGS) -o hungry bin/hungrysnakes.o bin/util.o $(SNAKE_LIBS)

nums: bin/numbersmain.o bin/util.o $(LWP_LIB) 
	$(LD) $(LDFLAGS) -o nums bin/numbersmain.o $(LWP_LIB_FLAG)

bin/hungrysnakes.o: demos/hungrysnakes.c include/lwp.h include/snakes.h
	$(CC) $(CFLAGS) -c demos/hungrysnakes.c -o $@

bin/randomsnakes.o: demos/randomsnakes.c include/lwp.h include/snakes.h
	$(CC) $(CFLAGS) -c demos/randomsnakes.c -o $@

bin/numbersmain.o: demos/numbersmain.c include/lwp.h
	$(CC) $(CFLAGS) -c demos/numbersmain.c -o $@

bin/util.o: demos/util.c include/lwp.h include/util.h include/snakes.h
	$(CC) $(CFLAGS) -c demos/util.c -o $@

# =====================================================================

$(LWP_LIB): bin/lwp.o
	$(CC) $(CFLAGS) -shared -o $@ bin/lwp.o 

bin/lwp.o: src/lwp.c
	$(CC) $(CFLAGS) -c $< -o $@

