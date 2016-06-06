#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MiniSQL.h"
#include "Record.h"
#include "BPlusTree.h"
#define TABLE_META_OFFSET 0
#define TABLE_META_DATA_FILENAME "table_meta.db"

int CreateTable(Table table)
{
    //@TODO we need Catalog manager here to check whether in the database
    FILE *fp;
    char fileName[256];
    char metaFileName[] = TABLE_META_DATA_FILENAME;
    // append the meta data to the meta data file
    fp = fopen(metaFileName, "ab+");
    WriteBlock(metaFileName, table, TABLE_META_OFFSET, sizeof(struct TableRecord));
    fclose(fp);

    strcpy(fileName, table->name);
    strcat(fileName, "_record.db");
    fp = fopen(fileName, "wb");
    fclose(fp);
    return 0;
}

int DropTable(Table table)
{
    return 0;
}
