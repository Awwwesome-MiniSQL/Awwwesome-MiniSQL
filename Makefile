CC = gcc
CFLAGS = -Wall -c
LFLAGS = -Wall
DEBUG = -g
OBJS = BPlusTreeInt.o BPlusTreeFloat.o BPlusTreeStr.o BPlusTree.o Record.o test.o
OUTPUT = -o test

all: test

test: $(OBJS)
	$(CC) $(LFLAGS) $(DEBUG) $(OBJS) $(OUTPUT)

BPlusTree.o: BPlusTree.c BPlusTree.h MiniSQL.h
	$(CC) $(CFLAGS) $(DEBUG) BPlusTree.c

BPlusTreeInt.o: BPlusTreeInt.c BPlusTreeInt.h BPlusTree.c BPlusTree.h MiniSQL.h
	$(CC) $(CFLAGS) $(DEBUG) BPlusTreeInt.c

BPlusTreeFloat.o: BPlusTreeFloat.c BPlusTreeFloat.h BPlusTree.c BPlusTree.h MiniSQL.h
	$(CC) $(CFLAGS) $(DEBUG) BPlusTreeFloat.c

BPlusTreeStr.o: BPlusTreeStr.c BPlusTreeStr.h BPlusTree.c BPlusTree.h MiniSQL.h
	$(CC) $(CFLAGS) $(DEBUG) BPlusTreeStr.c

Record.o: Record.c Record.h MiniSQL.h BPlusTree.h
	$(CC) $(CFLAGS) $(DEBUG) Record.c

test.o: test.c MiniSQL.h BPlusTree.h
	$(CC) $(CFLAGS) $(DEBUG) test.c

clean:
	rm -f $(OBJS) test
