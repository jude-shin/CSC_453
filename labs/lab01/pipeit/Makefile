CC = gcc
CFLAGS= -Wall -pedantic -g
LD = gcc
LDFLAGS =
SHELL=/bin/bash

MAIN = pipeit
OBJS = bin/main.o bin/family.o
PROGS = $(MAIN)

$(MAIN) : $(OBJS)
	$(LD) $(LDFLAGS) -o $(MAIN) $(OBJS)

bin/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm bin/*.o $(PROGS) outfile
