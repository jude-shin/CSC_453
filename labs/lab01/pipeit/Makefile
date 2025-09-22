CC = gcc
CFLAGS= -Wall -pedantic -g
LD = gcc
LDFLAGS =
SHELL=/bin/bash

MAIN = pipeit
OBJS = main.o family.o
PROGS = $(MAIN)

$(MAIN) : $(OBJS)
	$(LD) $(LDFLAGS) -o $(MAIN) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm *.o $(PROGS) outfile
