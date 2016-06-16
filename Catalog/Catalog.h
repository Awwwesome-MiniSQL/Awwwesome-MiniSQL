#ifndef CATALOG_H
#define CATALOG_H
#include "../MiniSQL.h"
// @TODO
// table's file name: TableName_record.db
// index file name: TableName_AttributeName_index.db
// each index corrensponds to a number, i.e., 0 -> t1_a1_index.db, 1 -> t2_a0_index.db ...

//@brief When a new table was created, we should check whether it is already in the catalog, if not, add the table to the catalog
int AddTableToCatalog(Table table);

//@brief when we drop a table from database, we drop the table in catalog
int RemoveTableFromCatalog(char *name);

// when we create an index file, add it to the catalog
int AddIndexToCatalog(char *name);

//@brief given the index number, find the index file name and put it in argument 2
void GetIndexFileName(int indexNum, char *indexFileName);

//@brief remove the index from Catalog when we drop a table or delete index
int RemoveIndexFromCatalog(int num);

#endif
