CC=/bin/gcc
CFLAGS=-Wall -g -fPIC

# ===================================================

libmalloc.so: bin/lwp.o
	$(CC) $(CFLAGS) -shared -o $@ lwp.o 

bin/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# ===================================================

clean:
	rm -f 
