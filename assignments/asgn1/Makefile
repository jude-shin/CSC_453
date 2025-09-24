CC = gcc
CFLAGS= -Wall -pedantic -g
LD = gcc
LDFLAGS =
SHELL=/bin/bash

MAIN = test 
OBJS = bin/test.o bin/alloc.o bin/unk.o
PROGS = $(MAIN)

$(MAIN): $(OBJS)
	$(LD) $(LDFLAGS) -o $(MAIN) $(OBJS)

bin/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm bin/*.o $(PROGS)

