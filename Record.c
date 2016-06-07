#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MiniSQL.h"
#include "Record.h"
#include "BPlusTree.h"

int CreateTable(Table table)
{
    //@TODO we need Catalog manager here to check whether in the database
    FILE *fp;
    char fileName[256];
    int i;
    //char metaFileName[] = TABLE_META_DATA_FILENAME;
    // append the meta data to the meta data file
    //fp = fopen(metaFileName, "ab+");
    //WriteBlock(metaFileName, table, TABLE_META_OFFSET, sizeof(struct TableRecord));
    //fclose(fp);
    table->recordNum = 0;
    for (i = 0; i < table->recordNum; i++)
    {
        if (intType == table->attributes[i].type || floatType == table->attributes[i].type)
        {
            table->attributes[i].size = 4;
        }
        else if (table->attributes[i].size > 256)
        {
            table->attributes[i].size = 256;
        }
        table->recordSize += table->attributes[i].size;
    }
    table->recordsPerBlock = BLOCK_SIZE / table->recordSize;
    strcpy(fileName, table->name);
    strcat(fileName, "_record.db");
    fp = fopen(fileName, "wb");
    fclose(fp);
    WriteBlock(fileName, table, TABLE_META_OFFSET, sizeof(struct TableRecord));
    return 0;
}

int RemoveTable(Table table)
{
    char fileName[256];
    strcpy(fileName, table->name);
    strcat(fileName, "_record.db");
    remove(fileName);
    //@TODO we need Catalog manager to help us remove meta data
    return 0;
}

int SearchTuples(Table table, IntFilter intFilter, FloatFilter floatFilter, StrFilter strFilter, int *projection)  // do linear scan
{
    int i, j, blockNum, tmpRecordsNum, isLastBlockNotFull;
    off_t offset;

    char *tmpTuple, *curBlock;
    char fileName[256];
    // print the header of table first
    PrintTableHeader(table, projection);
    strcpy(fileName, table->name);
    strcat(fileName, "_record.db");
    // compute number of blocks in table
    isLastBlockNotFull = table->recordNum % table->recordsPerBlock;
    blockNum = isLastBlockNotFull ? table->recordNum / table->recordsPerBlock + 1 : table->recordNum / table->recordsPerBlock;
    for (j = 1; j <= blockNum; j++)  // for each block
    {
        tmpRecordsNum = (j == blockNum && isLastBlockNotFull) ? isLastBlockNotFull : table->recordsPerBlock;  // number of records in current block
        offset = TABLE_RECORD_OFFSET + (j - 1) * BLOCK_SIZE;
        curBlock = (char *)ReadBlock(fileName, offset, BLOCK_SIZE);
        for (i = 0; i < tmpRecordsNum; i++)  // for each record
        {
            tmpTuple = curBlock + tmpRecordsNum * table->recordSize;
            if (0 == CheckTuple(tmpTuple, table, intFilter, floatFilter, strFilter))
            {
                continue;
            }
            PrintTuple(table, tmpTuple, projection);
        }
#ifdef NOBUFFER
        free(curBlock);
#endif
    }
    return 0;
}

off_t InsertTuple(Table table, char *tuple)
{
    off_t offset, insertPos;
    char *node;
    char fileName[256];
    //@TODO maybe we need to search whether it's valid to insert the tuple
    strcpy(fileName, table->name);
    strcat(fileName, "_record.db");
    offset = TABLE_RECORD_OFFSET + table->recordNum / table->recordsPerBlock * BLOCK_SIZE;
    insertPos = table->recordNum % table->recordsPerBlock * table->recordSize;
#ifndef DEBUG
    printf("table->recordNum: %d\n", table->recordNum);
#endif
    if (table->recordNum % table->recordsPerBlock)
    {
        node = (char *)ReadBlock(fileName, offset, BLOCK_SIZE);
    }
    else
    {
        node = (char *)malloc(BLOCK_SIZE);
    }
    memcpy(node + insertPos, tuple, table->recordSize);
    WriteBlock(fileName, node, offset, BLOCK_SIZE);
#ifdef NOBUFFER
    free(node);
#endif
    table->recordNum++;
    WriteBlock(fileName, table, TABLE_META_OFFSET, BLOCK_SIZE);
    return offset + insertPos;
}

int IsValidToInsert(Table table, char *tuple)
{
    // @TODO first search in BPlusTree to see whether the primary is valid, then check those unique attributs
    return 0;
}

int IntAttrCmp(int attrValue, IntFilter filter)
{
    switch (filter->cond)
    {
        case EQUAL: return attrValue == filter->src;
        case NOTEQUAL: return attrValue != filter->src;
        case LARGER: return attrValue > filter->src;
        case LARGERE: return attrValue >= filter->src;
        case SMALLER: return attrValue < filter->src;
        case SMALLERE: return attrValue <= filter->src;
    }
    return 0;
}

int FloatAttrCmp(float attrValue, FloatFilter filter)
{
    switch (filter->cond)
    {
        case EQUAL: return attrValue == filter->src;
        case NOTEQUAL: return attrValue != filter->src;
        case LARGER: return attrValue > filter->src;
        case LARGERE: return attrValue >= filter->src;
        case SMALLER: return attrValue < filter->src;
        case SMALLERE: return attrValue <= filter->src;
    }
    return 0;
}

int StrAttrCmp(char *attrValue, StrFilter filter)
{
    int result;
    result = strcmp(attrValue, filter->src);
    switch (filter->cond)
    {
        case EQUAL: return result == 0;
        case NOTEQUAL: return result != 0;
        case LARGER: return result > 0;
        case LARGERE: return result >= 0;
        case SMALLER: return result < 0;
        case SMALLERE: return result <= 0;
    }
    return 0;
}

int CheckTuple(char *tmpTuple, Table table, IntFilter intFilter, FloatFilter floatFilter, StrFilter strFilter)
{
    int i, takeIt;
    int tmpIntNum;
    IntFilter curIF;
    float tmpFloatNum;
    FloatFilter curFF;
    char *tmpStr;
    StrFilter curSF;
    takeIt = 0;
    curIF = intFilter;
    curFF = floatFilter;
    curSF = strFilter;
    int attrOffset[MAX_ATTRIBUTE_NUM];
    if (NULL == intFilter && NULL == floatFilter && NULL == strFilter)  // select * from ...
    {
        return 1;
    }
    // compute each attribute's offset to use later
    attrOffset[0] = 0;
    for (i = 1; i < table->attrNum; i++)
    {
        attrOffset[i] = attrOffset[i - 1] + table->attributes[i - 1].size;
    }
    while (NULL != curIF)
    {
        tmpIntNum = *(int *)(tmpTuple + attrOffset[curIF->attrIndex]);
        if (0 == IntAttrCmp(tmpIntNum, curIF))
        {
            takeIt = 0;
            break;
        }
        takeIt = 1;
        curIF = curIF->next;
    }
    if (0 == takeIt)
    {
        return 0;
    }
    while (NULL != curFF)
    {
        tmpFloatNum = *(float *)(tmpTuple + attrOffset[curIF->attrIndex]);
        if (0 == FloatAttrCmp(tmpFloatNum, curFF))
        {
            takeIt = 0;
            break;
        }
        takeIt = 1;
        curFF = curFF->next;
    }
    if (0 == takeIt)
    {
        return 0;
    }
    while (NULL != curSF)
    {
        tmpStr = tmpTuple + attrOffset[curSF->attrIndex];
        if (0 == StrAttrCmp(tmpStr, curSF))
        {
            takeIt = 0;
            break;
        }
        takeIt = 1;
        curSF = curSF->next;
    }
    if (0 == takeIt)  // Not the tuple we want
    {
        return 0;
    }
    return 1;
}

void PrintTuple(Table table, char *tuple, int *projection)
{

}

void PrintTableHeader(Table table, int *projection)
{

}

int DeleteTuples(Table table, IntFilter intFilter, FloatFilter floatFilter, StrFilter strFilter)  // Delete a tuple and move the last tuple to fill the space
{
    int i, j, blockNum, tmpRecordsNum, isLastBlockNotFull;
    off_t offset;

    char *tmpTuple, *lastTuple, *curBlock, *lastBlock;
    char fileName[256];
    strcpy(fileName, table->name);
    strcat(fileName, "_record.db");
    // compute number of blocks in table
    isLastBlockNotFull = table->recordNum % table->recordsPerBlock;
    blockNum = isLastBlockNotFull ? table->recordNum / table->recordsPerBlock + 1 : table->recordNum / table->recordsPerBlock;
    for (j = 0; j < blockNum; j++)  // for each block
    {
        tmpRecordsNum = (j == blockNum - 1 && isLastBlockNotFull) ? isLastBlockNotFull : table->recordsPerBlock;  // number of records in current block
        offset = TABLE_RECORD_OFFSET + j * BLOCK_SIZE;
        curBlock = (char *)ReadBlock(fileName, offset, BLOCK_SIZE);
        for (i = 0; i < tmpRecordsNum; i++)  // for each record
        {
            tmpTuple = curBlock + tmpRecordsNum * table->recordSize;
            if (0 == CheckTuple(tmpTuple, table, intFilter, floatFilter, strFilter)) // not the tuple we are going to delete
            {
                continue;
            }
            // delete current tuple from the table
            if (j * table->recordsPerBlock + tmpRecordsNum + 1 != table->recordNum)  // the tuple to delete is not the last tuple, we move the last tuple to the
            {
                // @NOTE not going to truncate the file because it depends on systems
                if (j < blockNum - 1)  // not in the last block
                {
                    lastBlock = (char *)ReadBlock(fileName, TABLE_RECORD_OFFSET + (blockNum - 1) * BLOCK_SIZE, BLOCK_SIZE);
                }
                else
                {
                    lastBlock = curBlock;
                }
                lastTuple = lastBlock + (table->recordNum - 1) % table->recordsPerBlock * table->recordSize;
                memcpy(curBlock, lastTuple, table->recordSize);
            }
            table->recordNum--;
            isLastBlockNotFull = table->recordNum % table->recordsPerBlock;
            blockNum = isLastBlockNotFull ? table->recordNum / table->recordsPerBlock + 1 : table->recordNum / table->recordsPerBlock;
        }
#ifdef NOBUFFER
        free(curBlock);
#endif
    }
    return 0;
}
