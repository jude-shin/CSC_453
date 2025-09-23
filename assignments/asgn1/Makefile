CC = gcc
CFLAGS= -Wall -pedantic -g
LD = gcc
LDFLAGS =
SHELL=/bin/bash

MAIN = malloc 
OBJS = malloc.o chunk.o alloc.o
PROGS = $(MAIN)

# TODO: make an archive in this make

$(MAIN) : $(OBJS)
	$(LD) $(LDFLAGS) -o $(MAIN) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# TODO: clean the archive as well
clean :
	rm *.o $(PROGS)
