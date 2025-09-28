CC = gcc
CFLAGS= -Wall -g -fpic
SHELL=/bin/bash

intel-all: lib/libmalloc.so lib64/libmalloc.so

lib/libmalloc.so: lib alloc32.o chunk32.o
	$(CC) $(CFLAGS) -m32 -shared -o $@ alloc32.o chunk32.o

lib64/libmalloc.so: lib64 alloc64.o chunk64.o
	$(CC) $(CFLAGS) -m64 -shared -o $@ alloc64.o chunk64.o

lib: 
	mkdir lib

lib64: 
	mkdir lib64

%32.o: %.c
	$(CC) $(CFLAGS) -m32 -c $< -o $@

%64.o: %.c
	$(CC) $(CFLAGS) -m64 -c $< -o $@

clean:
	rm -f *.o lib/* lib64/*

