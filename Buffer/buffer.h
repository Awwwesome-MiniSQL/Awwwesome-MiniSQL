#ifndef BUFFER_H
#define BUFFER_H

#include "../Catalog/Catalog.h" 

int log2(long num);
int log2c(long num);

#define TOTAL_BASE (1024 * 1024 * 1024)
#define TOTAL_NUM (4)
#define BLOCK_SIZE (4 * 1024)     // 4 KB per page
#define BLOCK_BIT (log2c(BLOCK_SIZE))
#define TOTAL_BLOCK_NUM (TOTAL_BASE / BLOCK_SIZE * TOTAL_NUM)
#define TAG_BIT (log2c(TOTAL_BLOCK_NUM))
#define DATA_SIZE (4 * 1024 * 1024)           // 4 MB for buffer_data
#define DATA_NUM (DATA_SIZE / BLOCK_SIZE)
#define DATA_BIT (log2c(DATA_NUM))
#define PAGE_BIT (1 + 1 + TAG_BIT + DATA_BIT)
#define PAGE_SIZE (PAGE_BIT * TOTAL_BLOCK_NUM / 8)
#define BUFFER_PAGE_SIZE (1 * 1024 * 1024)    // 1 MB for buffer_page

#define PAGE2_NUM (PAGE_SIZE / BLOCK_SIZE)
// (v)   (page2_t)     (page_t)       (data)
//  1  page2  8
//   Memory[ + 3] | +2 | +1 | 0
#define DISK ("disk.db")
#define MDISK ("mdisk.db")

#define MAXINT (18)

typedef unsigned char byte;
typedef unsigned short half;
typedef unsigned long word;

struct TimeRecord
{
  word index;
  word time;
  struct TimeRecord *Next;
};
typedef struct TimeRecord TRecord;

extern TRecord *rHead, *rTail;
extern int MemUse[TOTAL_BASE / BLOCK_SIZE * TOTAL_NUM]; //

extern byte *page2v;
extern byte *page2t;
extern byte *PageBuffer;
extern byte *Buffer;

// first used in the run
void initMemory();

// should be used at the end of the
void freeMemory();

// give the name of the table and the block number, return the address of data in the buffer
word ReadBlock(char *name, word num);

// give the name of the table, the block number and the write block, write the block
int WriteBlock(char *name, word num, byte *block);



void freeMemory();
void initMemory();
void createDataBase(void);
void freeMemory();
int LRU();
word getMemoryAddr(word addr);
byte lb(word addr);
void sb(word addr, byte data);
half lh(word addr);
void sh(word addr, half data);
word lw(word addr);
void sw(word addr, word data);
word getFat(char *name, word num);

#endif
