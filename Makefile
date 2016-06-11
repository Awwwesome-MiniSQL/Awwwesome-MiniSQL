CC = gcc
CFLAGS = -Wall -c
LFLAGS = -Wall
DEBUG = -g
OBJS = BPlusTreeInt.o BPlusTreeFloat.o BPlusTreeStr.o BPlusTree.o Record.o Catalog.o test.o
OUTPUT = -o test

all: test

test: $(OBJS)
	$(CC) $(LFLAGS) $(DEBUG) $(OBJS) $(OUTPUT)

BPlusTree.o: BPlusTree/BPlusTree.c BPlusTree/BPlusTree.h MiniSQL.h
	$(CC) $(CFLAGS) $(DEBUG) BPlusTree/BPlusTree.c

BPlusTreeInt.o: BPlusTree/BPlusTreeInt.c BPlusTree/BPlusTreeInt.h BPlusTree/BPlusTree.c BPlusTree/BPlusTree.h MiniSQL.h
	$(CC) $(CFLAGS) $(DEBUG) BPlusTree/BPlusTreeInt.c

BPlusTreeFloat.o: BPlusTree/BPlusTreeFloat.c BPlusTree/BPlusTreeFloat.h BPlusTree/BPlusTree.c BPlusTree/BPlusTree.h MiniSQL.h
	$(CC) $(CFLAGS) $(DEBUG) BPlusTree/BPlusTreeFloat.c

BPlusTreeStr.o: BPlusTree/BPlusTreeStr.c BPlusTree/BPlusTreeStr.h BPlusTree/BPlusTree.c BPlusTree/BPlusTree.h MiniSQL.h
	$(CC) $(CFLAGS) $(DEBUG) BPlusTree/BPlusTreeStr.c

Record.o: Record/Record.c Record/Record.h MiniSQL.h BPlusTree/BPlusTree.h
	$(CC) $(CFLAGS) $(DEBUG) Record/Record.c

Catalog.o: Catalog/Catalog.c Catalog/Catalog.h
	$(CC) $(CFLAGS) $(DEBUG) Catalog/Catalog.c

test.o: test.c MiniSQL.h BPlusTree/BPlusTree.h
	$(CC) $(CFLAGS) $(DEBUG) test.c

clean:
	rm -f $(OBJS) test