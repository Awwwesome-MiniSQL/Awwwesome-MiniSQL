#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "MiniSQL.h"
#include "Record/Record.h"
#include "BPlusTree/BPlusTree.h"
#include "BPlusTree/BPlusTreeInt.h"
#include "BPlusTree/BPlusTreeFloat.h"
#include "BPlusTree/BPlusTreeStr.h"
#include "Catalog/Catalog.h"

int main()
{
    struct TableRecord table;
    int i;
    // get input from interpreter
    // attribute 0: name
    strcpy(table.name, "student");
    strcpy(table.attributes[0].name, "ID");
    table.attrNum = 0;
    table.attributes[0].type = floatType;
    table.attributes[0].size = 4;
    table.attributes[0].unique = 1;
    table.attributes[0].index = 0;
    table.attrNum++;
    // attribute 1: ID
    strcpy(table.attributes[1].name, "name");
    table.attributes[1].type = stringType;
    table.attributes[1].size = 4;
    table.attributes[1].unique = 0;
    table.attributes[1].index = -1;
    table.primaryKey = 0;
    table.recordSize = 0;
    table.attrNum++;
    // compute record
    for (i = 0; i < table.attrNum; i++)
    {
        table.recordSize += table.attributes[i].size;
    }
    printf("Create a table ...\n");
    CreateTable(&table);
    char *tuple = (char *)malloc(table.recordSize);
    // insert int primary key
    /*
    for (i = 0; i < 1000; i++)
    {
        *(int *)tuple = i;
        *(int *)(tuple + 4) = i;
        InsertTuple(&table, tuple);
    }
    *(int *)tuple = 4;
    *(int *)(tuple + 4) = 998;
    InsertTuple(&table, tuple);
    free(tuple);
    */
    // insert float primary key
    /*
    for (i = 0; i < 10; i++)
    {
        *(int *)tuple = i;
        *(float *)(tuple + 4) = (float)(i + 0.1);
        InsertTuple(&table, tuple);
    }
    *(int *)tuple = 4;
    *(float *)(tuple + 4) = (float)(998 + 0.1);
    InsertTuple(&table, tuple);
    */

    printf("Insert tuples one by one ...\n");
    for (i = 0; i < 100; i++)
    {
        *(float *)tuple = (float)i;
        strcpy(tuple + 4, "ABC");
        InsertTuple(&table, tuple);
    }
    i = 4;
    *(int *)tuple = i;
    strcpy(tuple + 4, "A");
    struct IntFilterType intF, *nextIF;
    intF.attrIndex = 0;
    intF.cond = LARGER;
    intF.src = 4;
    nextIF = (struct IntFilterType *)malloc(sizeof(struct IntFilterType));
    intF.next = nextIF;
    nextIF->attrIndex = 0;
    nextIF->cond = SMALLERE;
    nextIF->src = 7;
    nextIF->next = NULL;
    printf("minisql> select * from student\n");
    SearchTuples(&table, NULL, NULL, NULL, NULL);
    printf("minisql> select * from student where ID > 4 and ID <= 7\n");
    SearchTuples(&table, &intF, NULL, NULL, NULL);
    printf("minisql> delete from student where ID > 4 and ID <= 7\n");
    DeleteTuples(&table, &intF, NULL, NULL);
    //InsertTuple(&table, tuple);

    //RemoveTable(&table);
    //printf("recordsPerBlock: %d\n", table.recordsPerBlock);
    free(tuple);
    printf("minisql> select * from student\n");
    SearchTuples(&table, NULL, NULL, NULL, NULL);
    free(nextIF);
    return 0;
}
