CC = gcc
FLAGS= -Wall -pedantic -g

MAIN = pipeit.out
OBJS = bin/main.o bin/family.o
PROGS = $(MAIN)

all : $(PROGS)

bin/%.o: src/%.c
	$(CC) $(FLAGS) -c $< -o $@

$(MAIN) : $(OBJS)
	$(CC) $(FLAGS) -o $(MAIN) $(OBJS)

clean :
	rm bin/*.o $(PROGS) core
