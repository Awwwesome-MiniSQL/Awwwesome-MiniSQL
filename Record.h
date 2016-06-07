#ifndef RECORD_H
#define RECORD_H
#include <stdio.h>
#include "MiniSQL.h"
#define DEBUG
#define TABLE_META_OFFSET 0
#define TABLE_RECORD_OFFSET TABLE_META_OFFSET + BLOCK_SIZE
#define TABLE_META_DATA_FILENAME "table_meta.db"
enum CmpCond{EQUAL, NOTEQUAL, LARGER, SMALLER, LARGERE, SMALLERE};

typedef struct IntFilterType *IntFilter;
struct IntFilterType
{
    int attrIndex;
    enum CmpCond cond;
    int src;
    IntFilter next;
};

typedef struct FloatFilterType *FloatFilter;
struct FloatFilterType
{
    int attrIndex;
    enum CmpCond cond;
    float src;
    FloatFilter next;
};

typedef struct StrFilterType *StrFilter;
struct StrFilterType
{
    int attrIndex;
    enum CmpCond cond;
    char src[256];
    StrFilter next;
};

int CreateTable(Table table);
int RemoveTable(Table table);
int SearchTuples(Table table, IntFilter intFilter, FloatFilter floatFilter, StrFilter strFilter);
off_t InsertTuple(Table table, char *tuple);
int IsValidToInsert(Table table, char *tuple);
int DeleteTuples(Table table, IntFilter intFilter, FloatFilter floatFilter, StrFilter strFilter);  // Delete a tuple and move the last tuple to fill the space
int IntAttrCmp(int attrValue, IntFilter filter);
int FloatAttrCmp(float attrValue, FloatFilter filter);
int StrAttrCmp(char *attrValue, StrFilter filter);
int CheckTuple(char *tmpTuple, Table table, IntFilter intFilter, FloatFilter floatFilter, StrFilter strFilter);
void PrintTuple(Table table, char *tuple, int *projection);
void PrintTableHeader(Table table, int *projection);
#endif
