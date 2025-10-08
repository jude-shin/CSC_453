CC=/bin/gcc
CFLAGS=-Wall -g -fPIC

# ===================================================

libmalloc.so: lwp.o
	$(CC) $(CFLAGS) -shared -o $@ lwp.o 

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ===================================================

clean:
	rm -f *.o 
