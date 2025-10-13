CC=/bin/gcc
CFLAGS = -Wall -g -fPIC -I ./include
LD = /bin/gcc
LDFLAGS = -Wall -g -L lib64


LWP_LIB_FLAG = -lPLN
LWP_LIB = lib64/libPLN.so 

PROGS = snakes hungry nums 
ALL_PROGS = $(PROGS) my_snakes my_hungry my_nums

SNAKE_OBJS = bin/randomsnakes.o bin/util.o
SNAKE_LIBS = -lsnakes -lncurses -lrt
HUNGRY_OBJS = bin/hungrysnakes.o bin/util.o
NUM_OBJS = bin/numbersmain.o
TEST_OBJS = $(SNAKE_OBJS) $(HUNGRY_OBJS) $(NUM_OBJS)

EXTRA_CLEAN = core 

.PHONY: all allclean clean progs $(LWP_LIB)

# =====================================================================

all: $(ALL_PROGS)

cleanall: clean
	@rm -f $(EXTRACLEAN)
	rm -f $(ALL_PROGS) bin/lwp.o bin/roundrobin.o bin/magic64.o

clean:	
	rm -f $(TEST_OBJS) *~ TAGS

progs: $(PROGS)

# =====================================================================

snakes: bin/randomsnakes.o bin/util.o bin/roundrobin.o lib64/libsnakes.so 
	$(LD) $(LDFLAGS) -o $@ $(filter-out %.so, $^) -lPLN $(SNAKE_LIBS)

hungry: bin/hungrysnakes.o bin/util.o bin/roundrobin.o lib64/libsnakes.so
	$(LD) $(LDFLAGS) -o $@ $(filter-out %.so, $^) -lPLN $(SNAKE_LIBS)

nums: bin/numbersmain.o bin/roundrobin.o lib64/libsnakes.so
	$(LD) $(LDFLAGS) -o $@ $(filter-out %.so, $^) -lPLN

# =====================================================================

# my_snakes: bin/randomsnakes.o bin/util.o bin/roundrobin.o bin/magic64.o lib64/liblwp.so 
# 	$(LD) $(LDFLAGS) -o $@ $(filter-out %.so, $^) -llwp $(SNAKE_LIBS)
# 
# my_hungry: bin/hungrysnakes.o bin/util.o bin/roundrobin.o bin/magic64.o lib64/liblwp.so
# 	$(LD) $(LDFLAGS) -o $@ $(filter-out %.so, $^) -llwp $(SNAKE_LIBS)
# 
# my_nums: bin/numbersmain.o bin/roundrobin.o bin/magic64.o lib64/liblwp.so 
# 	$(LD) $(LDFLAGS) -o $@ $(filter-out %.so, $^) -llwp

my_snakes: bin/randomsnakes.o bin/util.o bin/roundrobin.o bin/magic64.o bin/lwp.o
	$(LD) $(LDFLAGS) -o $@ $^ $(SNAKE_LIBS)

my_hungry: bin/hungrysnakes.o bin/util.o bin/roundrobin.o bin/magic64.o bin/lwp.o
	$(LD) $(LDFLAGS) -o $@ $^ $(SNAKE_LIBS)

my_nums: bin/numbersmain.o bin/roundrobin.o bin/magic64.o bin/lwp.o
	$(LD) $(LDFLAGS) -o $@ $^

# =====================================================================

bin/hungrysnakes.o: demos/hungrysnakes.c include/lwp.h include/snakes.h
	$(CC) $(CFLAGS) -c $< -o $@

bin/randomsnakes.o: demos/randomsnakes.c include/lwp.h include/snakes.h
	$(CC) $(CFLAGS) -c $< -o $@

bin/numbersmain.o: demos/numbersmain.c include/lwp.h
	$(CC) $(CFLAGS) -c $< -o $@

bin/util.o: demos/util.c include/lwp.h include/util.h include/snakes.h
	$(CC) $(CFLAGS) -c $< -o $@

# =====================================================================
lib64/liblwp.so: bin/lwp.o
	$(CC) $(CFLAGS) -shared -o $@ $< 

bin/lwp.o: src/lwp.c
	$(CC) $(CFLAGS) -c $< -o $@

bin/roundrobin.o: src/roundrobin.c
	$(CC) $(CFLAGS) -c $< -o $@

# =====================================================================
bin/magic64.o: src/magic64.S
	$(CC) $(CFLAGS) -c $< -o $@
