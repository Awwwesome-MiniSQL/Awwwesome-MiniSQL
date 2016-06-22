#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../MiniSQL.h"
#include "Record.h"
#include "../Catalog/Catalog.h"
#include "../BPlusTree/BPlusTree.h"
#include "../BPlusTree/BPlusTreeInt.h"
#include "../BPlusTree/BPlusTreeFloat.h"
#include "../BPlusTree/BPlusTreeStr.h"

int CreateTable(Table table)
{
    int i;
    //@TODO we need Catalog manager here to check whether the table exists
    FILE *fp;
    char fileName[MAX_STRING_LENGTH];
    //int i;
    //char metaFileName[] = TABLE_META_DATA_FILENAME;
    // append the meta data to the meta data file
    //fp = fopen(metaFileName, "ab+");
    //WriteBlock(metaFileName, table, TABLE_META_OFFSET, sizeof(struct TableRecord));
    //fclose(fp);
    table->recordNum = 0;
    for (i = 0; i < table->attrNum; i++)
    {
        if (intType == table->attributes[i].type || floatType == table->attributes[i].type)
        {
            table->attributes[i].size = 4;
        }
        else if (table->attributes[i].size > MAX_STRING_LENGTH)
        {
            table->attributes[i].size = MAX_STRING_LENGTH;
        }
        table->recordSize += table->attributes[i].size;
    }
    table->recordsPerBlock = BLOCK_SIZE / table->recordSize;
    if (table->primaryKey >= 0)
    {
        table->attributes[table->primaryKey].index = 1;
    }
    sprintf(fileName, "%s_record.db", table->name);
    fp = fopen(fileName, "wb");
    fclose(fp);
    WriteBlock(fileName, table, TABLE_META_OFFSET, sizeof(struct TableRecord));
    // initialize a BPlusTree
    struct tree_t tree;
    if (table->primaryKey >= 0)
    {
        sprintf(fileName, "%s_%s_index.db", table->name, table->attributes[table->primaryKey].name);
        fp = fopen(fileName, "wb");
        fclose(fp);
        // @TODO ask Catalog to add the index
        InitTree(&tree, fileName, table->attributes[table->primaryKey].type);
    }
    // @NOTE output information
    printf("\nCreated table \"%s\".\n", table->name);
    return 0;
}

int RemoveTable(Table table)
{
    char fileName[MAX_STRING_LENGTH];
    int i;
    char indexFileName[MAX_NAME_LENGTH];
    sprintf(fileName, "%s_record.db", table->name);
    FILE *fp;
    fp = fopen(fileName, "r");
    if (NULL == fp)
    {
        printf("[Error] Table \"%s\" not found!\n", table->name);
        return 1;
    }
    fclose(fp);
    for (i = 0; i < table->attrNum; i++)
    {
        if (table->attributes[i].index >= 0)
        {
            // remove file
            // @TODO invoke Catalog to drop index
            sprintf(indexFileName, "%s_%s_index.db", table->name, table->attributes[i].name);
            fp = fopen(indexFileName, "r");
            if (NULL == fp)
            {
                continue;
            }
            fclose(fp);
            // @TODO update table's corresponding attribute index num
            remove(indexFileName);
        }
    }
    remove(fileName);
    //@TODO we need Catalog manager to help us remove meta data
    printf("Dropped table \"%s\".\n", table->name);
    return 0;
}

int SearchTuples(Table table, IntFilter intF, FloatFilter floatF, StrFilter strF, int *projection)  // do linear scan
{
    int i, count;
    int attrMaxLen[MAX_ATTRIBUTE_NUM], fullProjection[MAX_ATTRIBUTE_NUM];
    IntFilter curIF = intF;
    FloatFilter curFF = floatF;
    StrFilter curSF = strF;
    enum CmpCond cond;
    struct tree_t tree;
    my_key_t_int intKey;
    my_key_t_float floatKey;
    my_key_t_str strKey;

    count = 0;
    if (NULL == projection)
    {
        projection = fullProjection;
        for (i = 0; i < table->attrNum; i++)
        {
            projection[i] = i;
        }
    }
    // print the header of table first
    ComputeAttrsMaxLen(table, projection, attrMaxLen);
    PrintTableHeader(table, projection, attrMaxLen);
    // find the first index we can use to search
    int firstAttrWithIndex = -1;
    //int indexNum = -1;
    while (curIF && firstAttrWithIndex < 0)
    {
        if (table->attributes[curIF->attrIndex].index >= 0)
        {
            curFF = NULL;
            curSF = NULL;
            //indexNum = table->attributes[curIF->attrIndex].index;
            firstAttrWithIndex = curIF->attrIndex;
            cond = curIF->cond;
            break;
        }
        curIF = curIF->next;
    }
    while (curFF && firstAttrWithIndex < 0)
    {
        if (table->attributes[curFF->attrIndex].index >= 0)
        {
            curIF = NULL;
            curSF = NULL;
            //indexNum = table->attributes[curFF->attrIndex].index;
            firstAttrWithIndex = curFF->attrIndex;
            cond = curFF->cond;
            break;
        }
        curFF = curFF->next;
    }
    while (curSF && firstAttrWithIndex < 0)
    {
        if (table->attributes[curSF->attrIndex].index >= 0)
        {
            curIF = NULL;
            curFF = NULL;
            //indexNum = table->attributes[curSF->attrIndex].index;
            firstAttrWithIndex = curSF->attrIndex;
            cond = curSF->cond;
            break;
        }
        curSF = curSF->next;
    }
    // search with index or linear scan
    if (firstAttrWithIndex >= 0 && NOTEQUAL != cond && (intF || floatF || strF))  // use index
    {
        sprintf(tree.path, "%s_%s_index.db", table->name, table->attributes[firstAttrWithIndex].name);
        GetTree(&tree);
        // found the index file, then generate the key
        if (curIF)  // int key
        {
            intKey.key = curIF->src;
            count = TraverseSearch_int(table, projection, &tree, intKey, cond, attrMaxLen, intF, floatF, strF);
        }
        else if (curFF)
        {
            floatKey.key = curFF->src;
            count = TraverseSearch_float(table, projection, &tree,  floatKey, cond, attrMaxLen, intF, floatF, strF);
        }
        else
        {
            strcpy(strKey.key, curSF->src);
            count = TraverseSearch_str(table, projection, &tree, strKey, cond, attrMaxLen, intF, floatF, strF);
        }
    }
    else  // we have to do linear scan
    {
        count = LinearScan(table, projection, intF, floatF, strF, attrMaxLen);
    }
    PrintDashes(table, projection, attrMaxLen);
    printf("%d row(s) in set\n\n", count);
    return count;
}

off_t InsertTuple(Table table, char *tuple)
{
    off_t offset, insertPos;
    char *node;
    char fileName[MAX_STRING_LENGTH];
    sprintf(fileName, "%s_record.db", table->name);
    // compute the offset of the block of record to insert
    offset = TABLE_RECORD_OFFSET + table->recordNum / table->recordsPerBlock * BLOCK_SIZE;
    insertPos = table->recordNum % table->recordsPerBlock * table->recordSize;
    if (!IsValidToInsert(table, tuple, offset + insertPos))
    {
        printf("[ERROR] Cannot insert tuple because of constraints\n");
        return 0;
    }
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
    InsertTupleIndex(table, tuple, offset + insertPos);
    // @NOTE output info
    if (FLAG_RECORD_INFO)
    {
        printf("\n\nQuery OK, 1 row affected\n\n");
    }
    return offset + insertPos;
}

int IsValidToInsert(Table table, char *tuple, off_t offset)
{
    int i, attrOffset[MAX_ATTRIBUTE_NUM], found = 0;
    struct IntFilterType intF;
    struct FloatFilterType floatF;
    struct StrFilterType strF;
    ComputeAttrsOffset(table, attrOffset);
    for (i = 0; i < table->attrNum; i++)  // for each attribute
    {
        if (table->attributes[i].unique || table->primaryKey == i)  // if the attribute is a unique one or the primary key
        {
            // generate filter and search the unique attribute value
            switch (table->attributes[i].type)
            {
                case intType:
                    intF.attrIndex = i;
                    intF.cond = EQUAL;
                    intF.src = *(int *)(tuple + attrOffset[i]);
                    intF.next = NULL;
                    found = SearchUniqueAttr(table, &intF, NULL, NULL);
                    break;
                case floatType:
                    floatF.attrIndex = i;
                    floatF.cond = EQUAL;
                    floatF.src = *(float *)(tuple + attrOffset[i]);
                    floatF.next = NULL;
                    found = SearchUniqueAttr(table, NULL, &floatF, NULL);
                    break;
                case stringType:
                    strF.attrIndex = i;
                    strF.cond = EQUAL;
                    strcpy(strF.src, tuple + attrOffset[i]);
                    strF.next = NULL;
                    found = SearchUniqueAttr(table, NULL, NULL, &strF);
                    break;
            }
            if (found)
            {
                return 0;
            }
        }
    }

    return 1;
}

int IntAttrCmp(int attrValue, IntFilter filter)
{
    switch (filter->cond)
    {
        case EQUAL: return attrValue == filter->src; break;
        case NOTEQUAL: return attrValue != filter->src; break;
        case LARGER: return attrValue > filter->src; break;
        case LARGERE: return attrValue >= filter->src; break;
        case SMALLER: return attrValue < filter->src; break;
        case SMALLERE: return attrValue <= filter->src; break;
    }
    return 0;
}

int FloatAttrCmp(float attrValue, FloatFilter filter)
{
    switch (filter->cond)
    {
        case EQUAL: return attrValue == filter->src; break;
        case NOTEQUAL: return attrValue != filter->src; break;
        case LARGER: return attrValue > filter->src; break;
        case LARGERE: return attrValue >= filter->src; break;
        case SMALLER: return attrValue < filter->src; break;
        case SMALLERE: return attrValue <= filter->src; break;
    }
    return 0;
}

int StrAttrCmp(char *attrValue, StrFilter filter)
{
    int result;
    result = strcmp(attrValue, filter->src);
    switch (filter->cond)
    {
        case EQUAL: return result == 0; break;
        case NOTEQUAL: return result != 0; break;
        case LARGER: return result > 0; break;
        case LARGERE: return result >= 0; break;
        case SMALLER: return result < 0; break;
        case SMALLERE: return result <= 0; break;
    }
    return 0;
}

int CheckTuple(char *tmpTuple, Table table, IntFilter intF, FloatFilter floatF, StrFilter strF)
{
    int takeIt;
    int tmpIntNum;
    IntFilter curIF;
    double tmpFloatNum;
    FloatFilter curFF;
    char *tmpStr;
    StrFilter curSF;
    takeIt = 0;
    curIF = intF;
    curFF = floatF;
    curSF = strF;
    int attrOffset[MAX_ATTRIBUTE_NUM];
    ComputeAttrsOffset(table, attrOffset);
    if (NULL == intF && NULL == floatF && NULL == strF)  // select * from ...
    {
        return 1;
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
    if (NULL != curIF && 0 == takeIt)
    {
        return 0;
    }
    while (NULL != curFF)
    {
        tmpFloatNum = *(float *)(tmpTuple + attrOffset[curFF->attrIndex]);
        if (0 == FloatAttrCmp(tmpFloatNum, curFF))
        {
            takeIt = 0;
            break;
        }
        takeIt = 1;
        curFF = curFF->next;
    }
    if (NULL != curFF && 0 == takeIt)
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

int DeleteTuples(Table table, IntFilter intF, FloatFilter floatF, StrFilter strF)  // Delete a tuple and move the last tuple to fill the space
{
    int i, j, blockNum, tmpRecordsNum, isLastBlockNotFull, count = 0;
    off_t offset;

    char *tmpTuple, *lastTuple, *curBlock, *lastBlock;
    char fileName[MAX_STRING_LENGTH];
    sprintf(fileName, "%s_record.db", table->name);
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
            tmpTuple = curBlock + i * table->recordSize;
            if (0 == CheckTuple(tmpTuple, table, intF, floatF, strF)) // not the tuple we are going to delete
            {
                continue;
            }
            count++;
            RemoveTupleIndex(table, tmpTuple);
            // delete current tuple from the table
            if (j * table->recordsPerBlock + i + 1 < table->recordNum)  // the tuple to delete is not the last tuple, we move the last tuple to the empty position
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
                // make sure that the last tuple will not be deleted
                while (1 == CheckTuple(lastTuple, table, intF, floatF, strF) && table->recordNum - 1 > j * table->recordsPerBlock + i)  // while the last tuple is to delete and current tuple != the last one
                {
                    count++;
                    RemoveTupleIndex(table, tmpTuple);
                    table->recordNum--;
                    isLastBlockNotFull = table->recordNum % table->recordsPerBlock;
                    blockNum = isLastBlockNotFull ? table->recordNum / table->recordsPerBlock + 1 : table->recordNum / table->recordsPerBlock;
                    tmpRecordsNum = (j >= blockNum - 1 && isLastBlockNotFull) ? isLastBlockNotFull : table->recordsPerBlock;  // number of records in current block
                    if (table->recordNum <= 1)  // only tmpTuple remains (which will be deleted, too), hence, the table is empty
                    {
                        table->recordNum = 0;
#ifdef NOBUFFER
                        if (lastBlock != curBlock)
                        {
                            free(lastBlock);
                        }
                        free(curBlock);
#endif
                        printf("Awwesome, Deletions OK, %d row(s) affected\n\n", count);
                        WriteBlock(fileName, table, TABLE_META_OFFSET, sizeof(struct TableRecord));
                        return count;
                    }
                    if (0 == table->recordNum % table->recordsPerBlock)
                    {
                        /*
                        if (blockNum > 1)
                        {
                            blockNum--;
                        }
                        */
#ifdef NOBUFFER
                        free(lastBlock);
#endif
                        lastBlock = (char *)ReadBlock(fileName, TABLE_RECORD_OFFSET + (blockNum - 1) * BLOCK_SIZE, BLOCK_SIZE);
                    }
                    lastTuple = lastBlock + (table->recordNum - 1) % table->recordsPerBlock * table->recordSize;
                }
                if (table->recordNum - 1 > j * table->recordsPerBlock + i)
                {
                    memcpy(tmpTuple, lastTuple, table->recordSize);
                    UpdateTupleIndex(table, tmpTuple, offset + i * table->recordSize);
                    WriteBlock(fileName, curBlock, offset, BLOCK_SIZE);
#ifdef NOBUFFER
                    if (lastBlock != curBlock)
                    {
                        free(lastBlock);
                    }
#endif
                }
            }
            table->recordNum--;
            if (0 == table->recordNum || j * table->recordsPerBlock + i + 1 >= table->recordNum)
            {
                break;
            }
            isLastBlockNotFull = table->recordNum % table->recordsPerBlock;
            blockNum = isLastBlockNotFull ? table->recordNum / table->recordsPerBlock + 1 : table->recordNum / table->recordsPerBlock;
            tmpRecordsNum = (j >= blockNum - 1 && isLastBlockNotFull) ? isLastBlockNotFull : table->recordsPerBlock;  // number of records in current block
        }
        if (0 == table->recordNum || j * table->recordsPerBlock + i + 1 >= table->recordNum)
        {
            break;
        }
#ifdef NOBUFFER
        free(curBlock);
#endif
    }
    printf("Deletions OK, %d row(s) affected.\n\n", count);
    WriteBlock(fileName, table, TABLE_META_OFFSET, sizeof(struct TableRecord));
    return count;
}

void PrintTableHeader(Table table, int *projection, int *attrMaxLen)
{
    int i;
    PrintDashes(table, projection, attrMaxLen);
    i = 0;
    fputc('|', stdout);
    while (projection[i] >= 0 && i < table->attrNum)
    {
        printf(" %-*s |", attrMaxLen[i], table->attributes[projection[i]].name);
        i++;
    }
    fputc('\n', stdout);
    PrintDashes(table, projection, attrMaxLen);
}

void PrintTuple(Table table, char *tuple, int *projection, int *attrMaxLen)
{
    int i, attrOffset[MAX_ATTRIBUTE_NUM];
    i = 0;
    ComputeAttrsOffset(table, attrOffset);
    fputc('|', stdout);
    while (projection[i] >= 0 && i < table->attrNum)
    {
        switch (table->attributes[projection[i]].type)
        {
            case intType: printf(" %*d |", attrMaxLen[i], *(int *)(tuple + attrOffset[projection[i]])); break;
            case floatType: printf(" %*.1f |", attrMaxLen[i], *(float *)(tuple + attrOffset[projection[i]])); break;
            case stringType: printf(" %*s |", attrMaxLen[i], tuple + attrOffset[projection[i]]); break;
        }
        i++;
    }
    fputc('\n', stdout);
}

void PrintDashes(Table table, int *projection, int *attrMaxLen)
{
    int i, j;
    i = 0;
    fputc('+', stdout);
    while (projection[i] >= 0 && i < table->attrNum)
    {
        for (j = 0; j < attrMaxLen[i] + 2; j++)
        {
            fputc('-', stdout);
        }
        fputc('+', stdout);
        i++;
    }
    fputc('\n', stdout);
}

void ComputeAttrsOffset(Table table, int *attrOffset)
{
    // compute each attribute's offset to use later
    int i;
    attrOffset[0] = 0;
    for (i = 1; i < table->attrNum; i++)
    {
        attrOffset[i] = attrOffset[i - 1] + table->attributes[i - 1].size;
    }
}

value_t SearchUniqueAttr(Table table, IntFilter intF, FloatFilter floatF, StrFilter strF)  // search whether the unique attribute value already exists in the table, if there exists an index, then use index to search, else do linear scan
{
    int i, j, blockNum, tmpRecordsNum, isLastBlockNotFull;
    off_t offset;
    value_t tupleOffset;
    struct tree_t tree;
    char *tmpTuple, *curBlock;
    char fileName[MAX_STRING_LENGTH];
    my_key_t_int intKey;
    my_key_t_float floatKey;
    my_key_t_str strKey;
    // first get the unique attribute we are going to search
    int firstAttrWithIndex  = -1;
    //int indexNum = -1;
    if (intF && firstAttrWithIndex < 0 && table->attributes[intF->attrIndex].index >= 0)
    {
        //indexNum = table->attributes[intF->attrIndex].index;
        firstAttrWithIndex = intF->attrIndex;
    }
    else if (floatF && firstAttrWithIndex < 0 && table->attributes[floatF->attrIndex].index >= 0)
    {
        //indexNum = table->attributes[floatF->attrIndex].index;
        firstAttrWithIndex = floatF->attrIndex;
    }
    else if (strF && firstAttrWithIndex && table->attributes[strF->attrIndex].index >= 0)
    {
        //indexNum = table->attributes[strF->attrIndex].index;
        firstAttrWithIndex = strF->attrIndex;
    }
    if (firstAttrWithIndex >= 0)
    {
        sprintf(tree.path, "%s_%s_index.db", table->name, table->attributes[table->primaryKey].name);
        GetTree(&tree);
        // found the index file, then generate the key
        if (intF)  // int key
        {
            intKey.key = intF->src;
            tupleOffset = SearchIndex(&tree, intKey);
        }
        else if (floatF)
        {
            floatKey.key = floatF->src;
            tupleOffset = SearchIndex(&tree, floatKey);
        }
        else
        {
            strcpy(strKey.key, strF->src);
            tupleOffset = SearchIndex(&tree, strKey);
        }
        return tupleOffset;
    }
    else  // we have to do linear scan
    {
        // first get file name of the table
        sprintf(fileName, "%s_record.db", table->name);
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
                tmpTuple = curBlock + i * table->recordSize;
                if (0 == CheckTuple(tmpTuple, table, intF, floatF, strF))  // not found
                {
                    continue;
                }
                return offset + i * table->recordSize;  // the offset of the tuple we found
            }
#ifdef NOBUFFER
            free(curBlock);
#endif
        }
    }  // no index
    return 0;  // unique value not found
}

void ComputeAttrsMaxLen(Table table, int *projection, int *attrMaxLen)
{
    int i = 0, fieldMaxLen;
    while ((NULL == projection || projection[i] >= 0) && i < table->attrNum)
    {
        if (stringType == table->attributes[projection[i]].type)
        {
            fieldMaxLen = table->attributes[projection[i]].size;
        }
        else
        {
            fieldMaxLen = NUM_MAX_SIZE;
        }
        attrMaxLen[i] = (int)strlen(table->attributes[projection[i]].name) > fieldMaxLen ? strlen(table->attributes[projection[i]].name) : fieldMaxLen;
        i++;
    }
}

int LinearScan(Table table, int *projection, IntFilter intF, FloatFilter floatF, StrFilter strF, int *attrMaxLen)
{
    int i, j, blockNum, tmpRecordsNum, isLastBlockNotFull, count = 0;
    off_t offset;
    char *tmpTuple, *curBlock;
    char fileName[MAX_STRING_LENGTH];
    sprintf(fileName, "%s_record.db", table->name);
    // compute number of blocks in table
    isLastBlockNotFull = table->recordNum % table->recordsPerBlock;
    if (0 == table->recordNum)
    {
        return count;
    }
    blockNum = isLastBlockNotFull ? table->recordNum / table->recordsPerBlock + 1 : table->recordNum / table->recordsPerBlock;
    for (j = 1; j <= blockNum; j++)  // for each block
    {
        tmpRecordsNum = (j == blockNum && isLastBlockNotFull) ? isLastBlockNotFull : table->recordsPerBlock;  // number of records in current block
        offset = TABLE_RECORD_OFFSET + (j - 1) * BLOCK_SIZE;
        curBlock = (char *)ReadBlock(fileName, offset, BLOCK_SIZE);
        for (i = 0; i < tmpRecordsNum; i++)  // for each record
        {
            tmpTuple = curBlock + i * table->recordSize;
            if (0 == CheckTuple(tmpTuple, table, intF, floatF, strF))
            {
                continue;
            }
            PrintTuple(table, tmpTuple, projection, attrMaxLen);
            count++;
        }
#ifdef NOBUFFER
        free(curBlock);
#endif
    }
    return count;
}

void InsertTupleIndex(Table table, char *tuple, off_t offset)
{
    int i, attrOffset[MAX_ATTRIBUTE_NUM];
    struct tree_t tree;
    my_key_t_int intKey;
    my_key_t_float floatKey;
    my_key_t_str strKey;
    ComputeAttrsOffset(table, attrOffset);
    for (i = 0; i < table->attrNum; i++)  // for each attribute
    {
        if (table->attributes[i].index >= 0 || table->primaryKey == i)  // if the attribute has an index
        {
            sprintf(tree.path, "%s_%s_index.db", table->name, table->attributes[i].name);
            GetTree(&tree);
            // generate filter and search the unique attribute value
            switch (table->attributes[i].type)
            {
                case intType:
                    intKey.key = *(int *)(tuple + attrOffset[i]);
                    InsertIndex(&tree, intKey, offset);
                    break;
                case floatType:
                    floatKey.key = *(float *)(tuple + attrOffset[i]);
                    InsertIndex(&tree, floatKey, offset);
                    break;
                case stringType:
                    strcpy(strKey.key, tuple + attrOffset[i]);
                    InsertIndex(&tree, strKey, offset);
                    break;
            }
        }
    }
}

void RemoveTupleIndex(Table table, char *tuple)
{
    int i, attrOffset[MAX_ATTRIBUTE_NUM];
    struct tree_t tree;
    my_key_t_int intKey;
    my_key_t_float floatKey;
    my_key_t_str strKey;
    ComputeAttrsOffset(table, attrOffset);
    for (i = 0; i < table->attrNum; i++)  // for each attribute
    {
        if (table->attributes[i].index >= 0)  // if the attribute has an index
        {
            sprintf(tree.path, "%s_%s_index.db", table->name, table->attributes[i].name);
            GetTree(&tree);
            // generate filter and search the unique attribute value
            switch (table->attributes[i].type)
            {
                case intType:
                    intKey.key = *(int *)(tuple + attrOffset[i]);
                    RemoveIndex(&tree, intKey);
                    break;
                case floatType:
                    floatKey.key = *(float *)(tuple + attrOffset[i]);
                    RemoveIndex(&tree, floatKey);
                    break;
                case stringType:
                    strcpy(strKey.key, tuple + attrOffset[i]);
                    RemoveIndex(&tree, strKey);
                    break;
            }
        }
    }
}

int GetTree(BPlusTree tree)
{
    meta_t *meta;
    //GetIndexFileName(indexNum, tree->path);
    if ('\0' == tree->path[0])  // Oops, an invalid index file
    {
        printf("[ERROR] index file not found. You need to make sure that after drop an index, the meta data of a table should be updated.\n");
        return 1;
    }
    // read the meta data of tree first
    meta = (meta_t *)ReadBlock(tree->path, META_OFFSET, BLOCK_SIZE);
    // @NOTE be careful, not sure it is correct or not
    memcpy((void *)&tree->meta, meta, sizeof(meta_t));
#ifdef NOBUFFER
    free(meta);
#endif
    return 0;
}

void UpdateTupleIndex(Table table, char *tuple, off_t newOffset)
{
    RemoveTupleIndex(table, tuple);
    InsertTupleIndex(table, tuple, newOffset);
}

off_t GetTuple(char *fileName, off_t tupleOffset, char **tuple, off_t recordsOffset, char **curBlock)
{
    if (recordsOffset != tupleOffset - tupleOffset % BLOCK_SIZE)
    {
        recordsOffset = tupleOffset - tupleOffset % BLOCK_SIZE;
        // read the block we found
#ifdef NOBUFFER
        free(*curBlock);
#endif
        *curBlock = (char *)ReadBlock(fileName, recordsOffset, BLOCK_SIZE);

    }
    *tuple = (char *)(*curBlock)+ tupleOffset % BLOCK_SIZE;
    return recordsOffset;
}

int CreateIndex(Table table, char *attrName)
{
    int i;
    for (i = 0; i < table->attrNum; i++)  // find the attribute
    {
        if (0 == strcmp(table->attributes[i].name, attrName))
        {
            if (!table->attributes[i].unique)  // not unique, return 1
            {
                printf("[Error] Could not create an index which is not a unique one\n");
                return 1;
            }
            break;
        }
    }
    if (i >= table->attrNum)  // could not find such attribute
    {
        printf("[Error] Attribute \"%s\" on table \"%s\" not found\n", attrName, table->name);
        return 1;
    }
    // InitTree
    char fileName[MAX_NAME_LENGTH];
    FILE *fp;
    struct tree_t tree;
    sprintf(fileName, "%s_%s_index.db", table->name, table->attributes[i].name);
    fp = fopen(fileName, "wb");
    fclose(fp);
    // @TODO ask Catalog to add the index
    InitTree(&tree, fileName, table->attributes[i].type);
    // do linear search to insert index
    LinearAddIndices(table, i, &tree);
    return 0;
}

int LinearAddIndices(Table table, int attrNum, BPlusTree tree)
{
    int i, j, blockNum, tmpRecordsNum, isLastBlockNotFull, count = 0;
    off_t offset;
    char *tmpTuple, *curBlock;
    char fileName[MAX_STRING_LENGTH];
    my_key_t_int intKey;
    my_key_t_float floatKey;
    my_key_t_str strKey;
    int attrOffset[MAX_ATTRIBUTE_NUM];
    ComputeAttrsOffset(table, attrOffset);
    sprintf(fileName, "%s_record.db", table->name);
    // compute number of blocks in table
    isLastBlockNotFull = table->recordNum % table->recordsPerBlock;
    if (0 == table->recordNum)
    {
        return count;
    }
    blockNum = isLastBlockNotFull ? table->recordNum / table->recordsPerBlock + 1 : table->recordNum / table->recordsPerBlock;
    for (j = 1; j <= blockNum; j++)  // for each block
    {
        tmpRecordsNum = (j == blockNum && isLastBlockNotFull) ? isLastBlockNotFull : table->recordsPerBlock;  // number of records in current block
        offset = TABLE_RECORD_OFFSET + (j - 1) * BLOCK_SIZE;
        curBlock = (char *)ReadBlock(fileName, offset, BLOCK_SIZE);
        for (i = 0; i < tmpRecordsNum; i++)  // for each record
        {
            tmpTuple = curBlock + i * table->recordSize;
            //PrintTuple(table, tmpTuple, projection, attrMaxLen);
            switch (table->attributes[attrNum].type)
            {
                case intType:
                    intKey.key = *(int *)(tmpTuple + attrOffset[attrNum]);
                    InsertIndex(tree, intKey, offset);
                    break;
                case floatType:
                    floatKey.key = *(float *)(tmpTuple + attrOffset[attrNum]);
                    InsertIndex(tree, floatKey, offset);
                    break;
                case stringType:
                    strcpy(strKey.key, tmpTuple + attrOffset[attrNum]);
                    InsertIndex(tree, strKey, offset);
                    break;
            }
            count++;
        }
#ifdef NOBUFFER
        free(curBlock);
#endif
    }
    printf("%d keys added\n", count);
    return count;
}

int RemoveIndexFile(char *tableName, char *attrName)
{
    char fileName[MAX_NAME_LENGTH];
    // @TODO invoke Catalog to drop index
    sprintf(fileName, "%s_%s_index.db", tableName, attrName);
    // remove file
    // @TODO update table's corresponding attribute index num
    remove(fileName);
    // update table's meta data
    int i;
    Table table;
    char tableFileName[MAX_NAME_LENGTH];
    sprintf(tableFileName, "%s_record.db", tableName);
    table = ReadBlock(tableFileName, TABLE_META_OFFSET, sizeof(struct TableRecord));
    if (NULL == table)
    {
        printf("[Error] No table named \"%s\"\n", tableName);
        return 1;
    }
    for (i = 0; i < table->attrNum; i++)
    {
        if (strcmp(table->attributes[i].name, attrName) == 0)
        {
            table->attributes[i].index = -1;
            WriteBlock(tableFileName, table, TABLE_META_OFFSET, sizeof(struct TableRecord));
            break;
        }
    }
    if (i == table->attrNum)
    {
        printf("[Error] No such index\n");
        return 1;
    }
    return 0;
}
