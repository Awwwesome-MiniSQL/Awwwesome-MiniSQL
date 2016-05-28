CC = gcc
CFLAGS = -Wall -c --std=gnu11
LFLAGS = -Wall --std=gnu11
DEBUG = -g
OBJS = BPlusTree.o test.o
OUTPUT = -o test

all: test

test: $(OBJS)
	$(CC) $(LFLAGS) $(DEBUG) $(OBJS) $(OUTPUT)

BPlusTree.o: BPlusTree.c BPlusTree.h
	$(CC) $(CFLAGS) $(DEBUG) BPlusTree.c

test.o: test.c MiniSQL.h BPlusTree.h
	$(CC) $(CFLAGS) $(DEBUG) test.c

clean:
	rm -f $(OBJS) test
