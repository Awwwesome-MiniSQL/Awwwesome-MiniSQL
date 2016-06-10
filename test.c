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
    table.attributes[0].unique = 0;
    table.attributes[0].index = -1;
    table.attrNum++;
    // attribute 1: ID
    strcpy(table.attributes[1].name, "ID");
    table.attributes[1].type = intType;
    table.attributes[1].size = 4;
    table.attributes[1].unique = 1;
    table.attributes[1].index = -1;
    table.primaryKey = 1;
    table.attrNum = 2;
    table.recordSize = 0;
    table.attrNum++;
    // compute record
    for (i = 0; i < table.attrNum; i++)
    {
        table.recordSize += table.attributes[i].size;
    }
    CreateTable(&table);
    char *tuple = (char *)malloc(table.recordSize);
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
    //RemoveTable(&table);
    printf("recordsPerBlock: %d\n", table.recordsPerBlock);
    return 0;
}
