CC=/bin/gcc
CFLAGS = -Wall -g -fPIC -I ./include

.PHONY: liblwp.so allclean clean

# =====================================================================

liblwp.so: lwp.o
	$(CC) $(CFLAGS) -shared -o $@ $< 

# =====================================================================

lwp.o: lwp.c lwp.h
	$(CC) $(CFLAGS) -c $< -o $@

roundrobin.o: roundrobin.c
	$(CC) $(CFLAGS) -c $< -o $@

# =====================================================================

allclean: clean
	rm -rf *.so

clean:
	rm -rf *.o

