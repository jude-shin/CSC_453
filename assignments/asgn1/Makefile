CC=gcc
CFLAGS=-Wall -g -fpic
SHELL=/bin/bash

INCLUDE_PP=-I /home/pn-cs453/Given/Asgn1/include 
LIBRARY_PP=-L /home/pn-cs453/Given/Asgn1/lib64

.PHONY: malloc

god: god.o libmalloc.a
	$(CC) -o god -L ./ $(LIBRARY_PP) god.o libmalloc.a -lpp 

god.o: god.c
	$(CC) $(CFLAGS) -c -o god.o $(INCLUDE_PP) god.c

malloc: libmalloc.a libmalloc.so

libmalloc.a: alloc.o chunk.o
	ar r libmalloc.a alloc.o chunk.o

libmalloc.so: alloc.o chunk.o
	$(CC) $(CFLAGS) -shared -o $@ alloc.o chunk.o

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE_PP) -c $< -o $@


# ===================================================

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
	$(CC) $(CFLAGS) -m32 $(INCLUDE_PP) -c $< -o $@

%64.o: %.c
	$(CC) $(CFLAGS) -m64 $(INCLUDE_PP) -c $< -o $@


# ===================================================

clean:
	rm -f *.o lib/* lib64/* libmalloc.a libmalloc.so testme god
