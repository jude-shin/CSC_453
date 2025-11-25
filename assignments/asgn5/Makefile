CC=/bin/gcc
CFLAGS = -Wall -g -pedantic -fPIC -I ./include
LD = /bin/gcc
LDFLAGS = -Wall -g -pedantic -lrt -lpthread

ALL_PROGS = minls minget
OBJS = minls.o minget.o disk.o

.PHONY: all allclean clean 

# =====================================================================

all: $(ALL_PROGS)

allclean: clean
	rm -f $(ALL_PROGS) 

clean:	
	rm -f $(OBJS)

# =====================================================================

minls: minls.o disk.o
	$(LD) $(LDFLAGS) -o $@ $^

minget: minget.o disk.o
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@


# =====================================================================
