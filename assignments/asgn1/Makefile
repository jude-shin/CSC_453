CC = gcc
CFLAGS= -Wall -pedantic -g -fpic
SHELL=/bin/bash
.PHONY: malloc

# malloc: libmalloc.a libmalloc.so
# 	$(CC) $(CFLAGS) -m32 -shared -o $@ bin/alloc32.o
	
intel-all: lib/libmalloc.so lib64

lib/libmalloc.so: lib bin/alloc32.o
	$(CC) $(CFLAGS) -m32 -shared -o $@ bin/alloc32.o

lib64/libmalloc.so: lib64 bin/alloc64.o
	$(CC) $(CFLAGS) -shared -o $@ bin/alloc64.o

lib: 
	mkdir lib

lib64: 
	mkdir lib64

bin/%32.o: src/%.c
	$(CC) $(CFLAGS) -m32 -c $< -o $@

bin/%64.o: src/%.c
	$(CC) $(CFLAGS) -m64 -c $< -o $@

# ----------------------------

tryme: test/tryme.o
	$(CC) -L ~/pn-cs453/demos/lib -o tryme test/tryme.o -lmalloc 

test/%.o: test/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf bin/ 

