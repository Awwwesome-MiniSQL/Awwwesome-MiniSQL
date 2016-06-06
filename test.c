#include <stdio.h>
#include <string.h>
#include "MiniSQL.h"
#include "Record.h"

int main()
{
    struct TableRecord table;
    int i;
    strcpy(table.name, "student");
    strcpy(table.attributes[0].name, "name");
    table.attrNum = 0;
    table.attributes[0].type = stringType;
    table.attributes[0].size = 30;
    table.attributes[0].unique = 0;
    table.attributes[0].index = -1;
    table.attrNum++;
    strcpy(table.attributes[1].name, "ID");
    table.attributes[1].type = intType;
    table.attributes[1].size = 4;
    table.attributes[1].unique = 1;
    table.attributes[1].index = -1;
    table.primaryKey = 1;
    table.recordNum = 2;
    table.recordSize = 0;
    table.attrNum++;
    for (i = 0; i < table.recordNum; i++)
    {
        table.recordSize += table.attributes[i].size;
    }
    //printf("lalala: %ld\n", sizeof(struct TableRecord));
    CreateTable(&table);
    return 0;
}
