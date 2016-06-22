#ifndef CATALOG_H
#define CATALOG_H

#include "../MiniSQL.h"

// calculate from fat to the real physical address 
#define TRUEADDR(ADDR) ((ADDR) * BLOCK_SIZE + 0x20000)

typedef unsigned char byte;
typedef unsigned short half;
typedef unsigned long word;

struct FileTree
{
  char name[MAX_NAME_LENGTH];
  word Addr;
  struct FileTree *Left, *Right;
};
typedef struct FileTree *FTree;

//@brief When a new table was created, we should check whether it is already in the catalog, if not, add the table to the catalog
int AddTableToCatalog(Table table);

//@brief when we drop a table from database, we drop the table in catalog
int RemoveTableFromCatalog(char *name);

// when we create an index file, add it to the catalog
int AddIndexToCatalog(char *tableName, int attributeNum, char *indexName);

//@brief remove the index from Catalog when we drop a table or delete index
int RemoveIndexFromCatalog(char *tableName, int attributeNum, char *indexName);



// the inner function 

// get a space from disk according to the fat table 
word findSpace();
// create a file Tree Node 
FTree createFile(char *str);
// find a Node named *str 
FTree findFile(char *str, FTree T);
// insert Node P 
FTree insertFile(FTree P, FTree T);
FTree findMin(FTree T);
// delete Node P 
FTree deleteFile(FTree P, FTree T);
// free Tree 
void freeFile(FTree T);

extern FTree THead;

#endif
