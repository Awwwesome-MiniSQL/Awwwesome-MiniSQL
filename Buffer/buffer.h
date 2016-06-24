#ifndef BUFFER_H
#define BUFFER_H

#include "../MiniSQL.h"
#include "../Catalog/Catalog.h"
int log2my(long num);
int log2c(long num);

typedef unsigned char byte;
typedef unsigned short half;
typedef unsigned long word;

// calculate the basic data to use
#define TOTAL_BASE (1024 * 1024 * 1024)
#define TOTAL_NUM (4)
#define BLOCK_BIT (log2c(BLOCK_SIZE))
#define TOTAL_BLOCK_NUM (TOTAL_BASE / BLOCK_SIZE * TOTAL_NUM)
#define TAG_BIT (log2c(TOTAL_BLOCK_NUM))
#define DATA_SIZE (4 * 1024 * 1024)           // 4 MB for buffer_data
#define DATA_NUM (DATA_SIZE / BLOCK_SIZE)
#define DATA_BIT (log2c(DATA_NUM))
#define PAGE_BIT (1 + 1 + TAG_BIT + DATA_BIT)
#define PAGE_SIZE (PAGE_BIT * TOTAL_BLOCK_NUM / 8)
#define BUFFER_PAGE_SIZE (1 * 1024 * 1024 * 4)    // 1 MB for buffer_page

#define PAGE2_NUM (PAGE_SIZE / BLOCK_SIZE)

// fat and data
#define DISK ("disk.db")
// the first class page
#define MDISK ("mdisk.db")
// the file Tree
#define FDISK ("fdisk.db")
#define FILE_BLOCK (64)
#define FILE_PNTLOC (sizeof(byte) * MAX_NAME_LENGTH + sizeof(word))

#define MAXINT (18)

struct TimeRecord
{
    word index;
    word time;
    struct TimeRecord *Next;
};
typedef struct TimeRecord TRecord;

// first used in the run
void initMemory();

// should be used at the end of the
void freeMemory();

// give the name of the table and the block number, return the address of data in the buffer
word ReadBlock(char *name, word num);

// give the name of the table, the block number and the write block, write the block
int WriteBlock(char *name, word num, byte *block);



// inner functions and inner variables

// the LRU data
extern TRecord *rHead, *rTail;
extern int MemUse[TOTAL_BASE / BLOCK_SIZE * TOTAL_NUM]; //

// buffer
// the second page for visible check
extern byte *page2v;
// the second page for the location of PageBuffer
extern byte *page2t;
// store the first class page
extern byte *PageBuffer;
// store the data
extern byte *Buffer;
extern word fileLoc;

// free the buffer and store data to the memory
void freeMemory();
// initial Memory and load file Tree if existed
void initMemory();
// create database files
int createDataBase(void);
// for LRU, return the least used Memory allocation
int LRU();
// turn the virtual address to the memory address
word getMemoryAddr(word addr);
// followings are data operations
byte lb(word addr);
void sb(word addr, byte data);
half lh(word addr);
void sh(word addr, half data);
word lw(word addr);
void sw(word addr, word data);
// find the expected block, like the block "num" of file "*name"
word getFat(char *name, word num);
// using deep first search to store and load files as follow functions
word dfsStore(FTree T, FILE *fp);
int storeFile(FTree T);
FTree dfsLoad(word fileAddr, FILE *fp);
FTree loadFile(word fileAddr);
// store the data in the buffer
void storeMemory(FTree T);

#endif
