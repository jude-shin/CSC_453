CC=/bin/gcc
CFLAGS = -Wall -g -fPIC -I ./include

.PHONY: liblwp.so allclean clean

# =====================================================================

liblwp.so: lwp.o roundrobin.o magic64.S
	$(CC) $(CFLAGS) -shared -o $@ $^

# =====================================================================

lwp.o: lwp.c lwp.h
	$(CC) $(CFLAGS) -c $< -o $@

roundrobin.o: roundrobin.c roundrobin.h
	$(CC) $(CFLAGS) -c $< -o $@

magic64.o: magic64.S
	$(CC) $(CFLAGS) -c $< -o $@

# =====================================================================

allclean: clean
	rm -rf *.so

clean:
	rm -rf *.o

