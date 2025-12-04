CC=/bin/gcc
CFLAGS = -Wall -g
LD = /bin/gcc
LDFLAGS = -Wall -g

ALL_PROGS = minls minget
OBJS = minls.o minget.o 
HELPER_OBJS	= disk.o print.o parse.o zone.o

.PHONY: all allclean clean 

# =====================================================================

all: $(ALL_PROGS)

allclean: clean
	rm -f $(ALL_PROGS) 

clean:	
	rm -f $(OBJS) $(HELPER_OBJS)

# =====================================================================

minls: minls.o $(HELPER_OBJS) 
	$(LD) $(LDFLAGS) -o $@ $^

minget: minget.o $(HELPER_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@


# =====================================================================
