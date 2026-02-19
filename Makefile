CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -g
LDFLAGS :=

SRC := pcb.c

all: mlq random sigtrap

mlq: mlq.o pcb.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

random: random.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

sigtrap: sigtrap.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

mlq.o: mlq.c pcb.h

pcb.o: pcb.c pcb.h

random.o: random.c

sigtrap.o: sigtrap.c

.PHONY: all clean

clean:
	rm -f *.o mlq random sigtrap

