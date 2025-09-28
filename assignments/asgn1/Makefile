CC=gcc
CFLAGS=-Wall -g -fpic
SHELL=/bin/bash

INCLUDE_PP=-I /home/pn-cs453/Given/Asgn1/include 
LIBRARY_PP=-L /home/pn-cs453/Given/Asgn1/lib64

# tryme: intel-all tryme.o
# 	$(CC) -L ./lib64 -o tryme tryme.o -lmalloc 
# 
# tryme.o:
# 	$(CC) $(CFLAGS) -c -o tryme.o tryme.c

god: intel-all god.o
	$(CC) -o god -L ./lib64 $(LIBRARY_PP) god.o -lmalloc -lpp 

god.o: god.c
	$(CC) $(CFLAGS) -c -o god.o $(INCLUDE_PP) god.c

intel-all: lib64/libmalloc.so

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

clean:
	rm -f *.o lib/* lib64/* testme tryme god
