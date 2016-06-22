#ifndef RECORD_H
#define RECORD_H
#include <stdio.h>
#include "../MiniSQL.h"
#include "../BPlusTree/BPlusTree.h"
#define DEBUG
#define TABLE_META_OFFSET 0
#define NUM_MAX_SIZE 12
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
    char src[MAX_STRING_LENGTH];
    StrFilter next;
};
// ================ other modules can invoke the following functions ===========
// @brief create a file XXX_record.db and write meta data into the first block of the file, return 0 if succeeded
int CreateTable(Table table);
// @brief simply remove the record file and we need catalog to update tables information, return 0 if succeeded
int RemoveTable(Table table);
// @brief search tuples and project, if projection == NULL, then select * from .., return 0 if succeeded
int SearchTuples(Table table, IntFilter intFilter, FloatFilter floatFilter, StrFilter strFilter, int *projection);
// @brief return the offset of the tuple in record file, return the offset of the tuple inserted
off_t InsertTuple(Table table, char *tuple);
// @brief Delete a tuple and move the last tuple to fill the space, return the number of affected rows
int DeleteTuples(Table table, IntFilter intFilter, FloatFilter floatFilter, StrFilter strFilter);
// @brief find the index file and store the meta data in tree, return 0 if succeeded
int GetTree(BPlusTree tree);
// @brief create an index file and insert all keys of tuples in the table, return 0 if succeeded
int CreateIndex(Table table, char *attrName);
// @brief remove index file, return 0 if succeeded
int RemoveIndexFile(char *tableName, char *attrName);

int InsertExecStart(Table table, char *data);
void InsertExecTuple(char *tuple);
// @brief after FastInsert, maintain table meta data and insert keys to B+ tree
int InsertExecStop();
// =============================================================================
// @brief try to insert the tuple in all related BPlusTree to test whether it can be inserted
int IsValidToInsert(Table table, char *tuple, off_t offset);
int IntAttrCmp(int attrValue, IntFilter filter);
int FloatAttrCmp(float attrValue, FloatFilter filter);
int StrAttrCmp(char *attrValue, StrFilter filter);
int CheckTuple(char *tmpTuple, Table table, IntFilter intFilter, FloatFilter floatFilter, StrFilter strFilter);
void PrintTuple(Table table, char *tuple, int *projection, int *attrMaxLen);
void PrintTableHeader(Table table, int *projection, int *attrMaxLen);
void PrintDashes(Table table, int *projection, int *attrMaxLen);
void ComputeAttrsOffset(Table table, int *attrOffset);
//@brief search whether the unique attribute value already exists in the table
value_t SearchUniqueAttr(Table table, IntFilter intFilter, FloatFilter floatFilter, StrFilter strFilter);
void ComputeAttrsMaxLen(Table table, int *projection, int *attrMaxLen);
int LinearScan(Table table, int *projection, IntFilter intF, FloatFilter floatF, StrFilter strF, int *attrMaxLen);
void InsertTupleIndex(Table table, char *tuple, off_t offset);
void RemoveTupleIndex(Table table, char *tuple);
void UpdateTupleIndex(Table table, char *tuple, off_t newOffset);
off_t GetTuple(char *fileName, off_t tupleOffset, char **tuple, off_t recordsOffset, char **curBlock);

int TraverseSearch_int(Table table, int *projection, BPlusTree tree, my_key_t_int key, enum CmpCond cond, int *attrMaxLen, IntFilter intF, FloatFilter floatF, StrFilter strF);
int Move2NextChild_int(BPlusTree tree, leaf_t_int **leaf, int i);
int Move2PreviousChild_int(BPlusTree tree, leaf_t_int **leaf, int i);

int TraverseSearch_float(Table table, int *projection, BPlusTree tree, my_key_t_float key, enum CmpCond cond, int *attrMaxLen, IntFilter intF, FloatFilter floatF, StrFilter strF);
int Move2NextChild_float(BPlusTree tree, leaf_t_float **leaf, int i);
int Move2PreviousChild_float(BPlusTree tree, leaf_t_float **leaf, int i);

int TraverseSearch_str(Table table, int *projection, BPlusTree tree, my_key_t_str key, enum CmpCond cond, int *attrMaxLen, IntFilter intF, FloatFilter floatF, StrFilter strF);
int Move2NextChild_str(BPlusTree tree, leaf_t_str **leaf, int i);
int Move2PreviousChild_str(BPlusTree tree, leaf_t_str **leaf, int i);

int LinearAddIndices(Table table, int attrNum, BPlusTree tree);

#endif
