CC=gcc
CFLAGS = -Wall -ansi -pedantic -g
LD=gcc
LDFLAGS = -g

.PHONY: clean test

hello: hello.o
	$(LD) $(LDFLAGS) -o $@ $<

hello.o: hello.c
	$(CC) $(CFLAGS) -c $< -o $@ 

clean:
	rm -f hello.o *~ TAGS

test: hello
	@echo "Testing hello..."
	@./hello
	@echo "done."
