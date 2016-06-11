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
    strcpy(table.attributes[0].name, "name");
    table.attrNum = 0;
    table.attributes[0].type = intType;
    table.attributes[0].size = 4;
    table.attributes[0].unique = 1;
    table.attributes[0].index = 0;
    table.attrNum++;
    // attribute 1: ID
    strcpy(table.attributes[1].name, "ID");
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

    for (i = 0; i < 10; i++)
    {
        *(int *)tuple = i;
        strcpy(tuple + 4, "ABC");
        InsertTuple(&table, tuple);
    }
    *(int *)tuple = 4;
    printf("Dengdeng\n");
    strcpy(tuple + 4, "A");
    struct IntFilterType intF;
    intF.attrIndex = 0;
    intF.cond = EQUAL;
    intF.src = 1;
    intF.next = NULL;
    SearchTuples(&table, NULL, NULL, NULL, NULL);
    SearchTuples(&table, &intF, NULL, NULL, NULL);
    DeleteTuples(&table, &intF, NULL, NULL);
    //InsertTuple(&table, tuple);

    //RemoveTable(&table);
    //printf("recordsPerBlock: %d\n", table.recordsPerBlock);
    free(tuple);
    SearchTuples(&table, NULL, NULL, NULL, NULL);
    return 0;
}
