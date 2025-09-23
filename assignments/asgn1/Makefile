CC = gcc
CFLAGS= -Wall -pedantic -g
LD = gcc
LDFLAGS =
SHELL=/bin/bash

MAIN = malloc 
OBJS = malloc.o
PROGS = $(MAIN)

$(MAIN) : $(OBJS)
	$(LD) $(LDFLAGS) -o $(MAIN) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm *.o $(PROGS)
