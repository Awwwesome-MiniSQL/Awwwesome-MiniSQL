#include "../MiniSQL.h"
#include "Record.h"
#include "../Catalog/Catalog.h"
#include "../BPlusTree/BPlusTree.h"
#include "../BPlusTree/BPlusTreeInt.h"

int TraverseSearch_int(Table table, int *projection, BPlusTree tree, my_key_t_int key, enum CmpCond cond, int *attrMaxLen, IntFilter intF, FloatFilter floatF, StrFilter strF)
{
    int i, compareRes, count = 0;
    char *tuple, *curBlock;
    leaf_t_int *leaf;
    off_t leafOffset, parent, recordsOffset;
    value_t tupleOffset;
    char fileName[MAX_STRING_LENGTH];
    sprintf(fileName, "%s_record.db", table->name);
    // first, find the key in leaf node
    parent = SearchIndex_int(tree, key);
    leafOffset = SearchLeaf_int(tree, parent, key);
    leaf = (leaf_t_int *)ReadBlock(tree->path, leafOffset, BLOCK_SIZE);
    for (i = 0; i < (int)leaf->n; i++)
    {
        compareRes = KeyCmp_int(leaf->children[i].key, key);
        if (compareRes >= 0)  // found its upper bound
        {
            break;
        }
    }
    // exception handling, if not found here
    if (i >= (int)leaf->n &&  (LARGERE == cond || LARGER == cond))  // look up the right block
    {
        if (0 == leaf->next)  // the end of the tree
        {
#ifdef NOBUFFER
                free(leaf);
#endif
            return count;
        }
        leafOffset = leaf->next;
        i = 0;
#ifdef NOBUFFER
        free(leaf);
#endif
        leaf = (leaf_t_int *)ReadBlock(tree->path, leafOffset, BLOCK_SIZE);
    }
    else if (0 == i && 0 != compareRes && (SMALLERE == cond || SMALLER == cond))  // look up the left block
    {
        if (0 == leaf->prev)
        {
#ifdef NOBUFFER
            free(leaf);
#endif
            return count;
        }
        leafOffset = leaf->prev;
        i = (int)leaf->n - 1;
#ifdef NOBUFFER
        free(leaf);
#endif
        leaf = (leaf_t_int *)ReadBlock(tree->path, leafOffset, BLOCK_SIZE);
    }

    tupleOffset = leaf->children[i].value;
    recordsOffset = tupleOffset - tupleOffset % BLOCK_SIZE;
    // read the block we found
    curBlock = (char *)ReadBlock(fileName, recordsOffset, BLOCK_SIZE);
    tuple = (char *)curBlock+ tupleOffset % BLOCK_SIZE;
    // if the compare condition is "==", ">=" or "<=" we might have to print the tuple whose offset == tupleOffset
    if ((EQUAL == cond || LARGERE == cond || SMALLERE == cond) && 0 != CheckTuple(tuple, table, intF, floatF, strF))
    {
        PrintTuple(table, tuple, projection, attrMaxLen);
        count++;
        if (LARGERE == cond)  // move to next children
        {
            i = Move2NextChild_int(tree, &leaf, i);
            if (i >= 0)
            {
                tupleOffset = leaf->children[i].value;
                recordsOffset = GetTuple(fileName, tupleOffset, &tuple, recordsOffset, &curBlock);
            }
        }
        else if (SMALLERE == cond)  // move to previous children
        {
            i = Move2PreviousChild_int(tree, &leaf, i);
            if (i >= 0)
            {
                tupleOffset = leaf->children[i].value;
                recordsOffset = GetTuple(fileName, tupleOffset, &tuple, recordsOffset, &curBlock);
            }
        }
        if (i < 0)  // no tuples to search any more
        {
#ifdef NOBUFFER
            free(leaf);
            free(curBlock);
#endif
            return count;
        }
    }
    switch (cond)
    {
        case LARGERE:
        case LARGER:  // treat LARGERE and LARGER as LARGER because EQUAL has been dealt with already
            while(i >= 0)
            {
                if (0 != CheckTuple(tuple, table, intF, floatF, strF))
                {
                    PrintTuple(table, tuple, projection, attrMaxLen);
                    count++;
                }
                i = Move2NextChild_int(tree, &leaf, i);
                if (i < 0)
                {
                    break;
                }
                tupleOffset = leaf->children[i].value;
                recordsOffset = GetTuple(fileName, tupleOffset, &tuple, recordsOffset, &curBlock);
            }
            break;
        case SMALLER:
        case SMALLERE: // treat SMALLERE and SMALLER as LARGER because EQUAL has been dealt with already
            while(i >= 0)
            {
                if (0 != CheckTuple(tuple, table, intF, floatF, strF))
                {
                    PrintTuple(table, tuple, projection, attrMaxLen);
                    count++;
                }
                i = Move2PreviousChild_int(tree, &leaf, i);
                if (i < 0)
                {
                    break;
                }
                tupleOffset = leaf->children[i].value;
                recordsOffset = GetTuple(fileName, tupleOffset, &tuple, recordsOffset, &curBlock);
            }
            break;
        case NOTEQUAL:
            LinearScan(table, projection, intF, floatF, strF, attrMaxLen);
            break;
        default: break;
    }
#ifdef NOBUFFER
    free(leaf);
    free(curBlock);
#endif
    return count;
}

int Move2NextChild_int(BPlusTree tree, leaf_t_int **leaf, int i)
{
    leaf_t_int *tmpLeaf = *leaf;
    if (i < (int)(*leaf)->n - 1)
    {
        return i + 1;
    }
    // have to read the next block
    if (0 == (*leaf)->next)  // no next block
    {
        return -1;
    }
    *leaf = (leaf_t_int *)ReadBlock(tree->path, (*leaf)->next, sizeof(leaf_t_int));
#ifdef NOBUFFER
    free(tmpLeaf);
#endif
    return 0;
}

int Move2PreviousChild_int(BPlusTree tree, leaf_t_int **leaf, int i)
{
    leaf_t_int *tmpLeaf = *leaf;
    if (i >= 0)
    {
        return i - 1;
    }
    // have to read the next block
    if (0 == (*leaf)->prev)  // no next block
    {
        return -1;
    }
    *leaf = (leaf_t_int *)ReadBlock(tree->path, (*leaf)->prev, sizeof(leaf_t_int));
#ifdef NOBUFFER
    free(tmpLeaf);
#endif
    return 0;
}
