#ifndef RECORD_H
#define RECORD_H
#include "MiniSQL.h"

enum CmpCond{EQUAL, NOTEQUAL, LARGER, SMALLER, LARGERE, SMALLERE};
typedef struct FilterType *Filter;
struct FilterType
{
    enum CmpCond cond;
    int src;
    Filter next;
};

int CreateTable(Table table);
int DropTable(Table table);
int SearchTuples(Table table, Filter filter);
int InsertTuple(Table table, void *tuple);
int DeleteTuples(Table table, Filter filter);
#endif
