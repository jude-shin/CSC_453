CC=/bin/gcc
CFLAGS=-Wall -g -fPIC

god: god.o libmalloc.so
	$(CC) -o god -L. god.o -lmalloc

god.o: god.c
	$(CC) $(CFLAGS) -c -o god.o god.c

# ===================================================

.PHONY: malloc

malloc: libmalloc.a libmalloc.so

libmalloc.a: alloc.o chunk.o
	ar r libmalloc.a alloc.o chunk.o

libmalloc.so: alloc.o chunk.o
	$(CC) $(CFLAGS) -shared -o $@ alloc.o chunk.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

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

alloc32.o: alloc.c
	$(CC) $(CFLAGS) -m32 -c -o alloc32.o alloc.c

chunk32.o: chunk.c
	$(CC) $(CFLAGS) -m32 -c -o chunk32.o chunk.c

alloc64.o: alloc.c
	$(CC) $(CFLAGS) -m64 -c -o alloc64.o alloc.c

chunk64.o: chunk.c
	$(CC) $(CFLAGS) -m64 -c -o chunk64.o chunk.c

# ===================================================

clean:
	rm -f *.o lib/* lib64/* libmalloc.a libmalloc.so core.* DETAILS.bak god


