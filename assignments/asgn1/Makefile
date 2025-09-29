CC  = gcc

CFLAGS = -Wall -g -fpic

intel-all: lib/libmalloc.so lib64/libmalloc.so

lib/libmalloc.so: lib alloc32.o chunk32.o
	$(CC) $(CFLAGS) -m32 -shared -o $@ alloc32.o chunk32.o

lib64/libmalloc.so: lib64 alloc64.o chunk32.o
	$(CC) $(CFLAGS) -shared -o $@ alloc64.o chunk64.o

lib: 
	mkdir lib

lib64:
	mkdir lib64

alloc32.o: alloc.c
	$(CC) $(CFLAGS) -m32 -c -o alloc32.o alloc.c

alloc64.o: alloc.c
	$(CC) $(CFLAGS) -m64 -c -o alloc64.o alloc.c

chunk32.o: chunk.c
	$(CC) $(CFLAGS) -m32 -c -o chunk32.o chunk.c

chunk64.o: chunk.c
	$(CC) $(CFLAGS) -m64 -c -o chunk64.o chunk.c
