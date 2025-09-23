CC = gcc
CFLAGS= -Wall -pedantic -g
LD = gcc
LDFLAGS =
SHELL=/bin/bash

OBJS = chunk.o alloc.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

malloc: 

libmalloc.a: 
	ar r libmalloc.a $(OBJS)

clean:
	rm *.o $(PROGS) libmalloc.a

test: $(OBJS)
	$(LD) $(LDFLAGS) -o test $(OBJS) test.o
