CC = gcc
CFLAGS = -Wall -c
LFLAGS = -Wall
DEBUG = -g
OBJS = Record.o test.o BPlusTree.o
OUTPUT = -o test

all: test

test: $(OBJS)
	$(CC) $(LFLAGS) $(DEBUG) $(OBJS) $(OUTPUT)

BPlusTree.o: BPlusTree.c BPlusTree.h MiniSQL.h
	$(CC) $(CFLAGS) $(DEBUG) BPlusTree.c

Record.o: Record.c Record.h MiniSQL.h BPlusTree.h
	$(CC) $(CFLAGS) $(DEBUG) Record.c

test.o: test.c MiniSQL.h Record.h
	$(CC) $(CFLAGS) $(DEBUG) test.c

clean:
	rm -f $(OBJS) test
