CC=/bin/gcc
CFLAGS = -Wall -ansi -pedantic
LD=/bin/gcc
LDFLAGS = -Wall -ansi -pedantic

.PHONY: clean test

bomb: bomb.o
	$(LD) $(LDFLAGS) -o $@ $<

bomb.o: bomb.c
	$(CC) $(CFLAGS) -c $< -o $@ 

clean:
	rm -f bomb.o

test: bomb 
	@./bomb
