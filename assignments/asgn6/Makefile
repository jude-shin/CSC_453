CC=/bin/gcc
CFLAGS = -Wall -ansi -pedantic
LD=/bin/gcc
LDFLAGS = -Wall -ansi -pedantic

.PHONY: clean test

hello: hello.o
	$(LD) $(LDFLAGS) -o $@ $<

hello.o: hello.c
	$(CC) $(CFLAGS) -c $< -o $@ 

clean:
	rm -f hello.o

test: hello
	@./hello
