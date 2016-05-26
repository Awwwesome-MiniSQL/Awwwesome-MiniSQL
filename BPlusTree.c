#include <string.h>
#include "BPlusTree.h"
// ======================= buffer read and write =======================
// ReadBlock and WriteBlock functions are provided by Buffer module
int ReadBlock(FILE *fp, void *block, off_t offset, size_t size)
{
    fseek(fp, offset, SEEK_SET);
    return fread(block, size, 1, fp) - 1;
}

int WriteBlock(FILE *fp, void *block, off_t offset, size_t size)
{
    fseek(fp, offset, SEEK_SET);
    return fwrite(block, size, 1, fp) - 1;
}

// This is my part
void OpenFile(BPlusTree tree)
{
    if (tree->fpLevel == 0)
    {
        fopen(tree->path, "rb+");
    }
    tree->fpLevel++;
}

void CloseFile(BPlusTree tree)
{
    if (tree->fpLevel == 1)
    {
        fclose(tree->fp);
    }
    tree->fpLevel--;
}

int ReadIndexBlock(BPlusTree tree, void *block, off_t offset, size_t size)
{
    int isDone;  // Successful or not
    OpenFile(tree);
    isDone = (ReadBlock(tree->fp, block, offset, size) == 0);
    CloseFile(tree);
    return isDone;
}

int WriteIndexBlock(BPlusTree tree, void *block, off_t offset, size_t size)
{
    int isDone;
    OpenFile(tree);
    isDone = (WriteBlock(tree->fp, block, offset, size) == 0);
    CloseFile(tree);
    return isDone;
}

//======================= show time =======================
// initialize tree
void InitTree(BPlusTree tree)
{
    internal_t root;
    leaf_t leaf;
    // initialize meta data
    tree->meta.order = TREE_ORDER;
    tree->meta.valueSize = sizeof(value_t);
    tree->meta.keySize = sizeof(key_t);
    tree->meta.internalNum = 0;
    tree->meta.leafNum = 0;
    tree->meta.height = 1;
    tree->meta.slot = BLOCK_OFFSET;
    // initialize root
    root.next = root.prev = root.parent = 0;
    tree->meta.rootOffset = AllocInternal(tree, &root);
    // initialize leaf
    leaf.next = leaf.prev = 0;
    leaf.parent = tree->meta.rootOffset;
    tree->meta.leafOffset = root.children[0].child = AllocLeaf(tree, &leaf);
    // write back to buffer
    WriteIndexBlock(tree, &tree->meta, META_OFFSET, sizeof(meta_t));
    WriteIndexBlock(tree, &root, tree->meta.rootOffset, sizeof(internal_t));
    WriteIndexBlock(tree, &leaf, tree->meta.leafOffset, sizeof(leaf_t));
}

off_t AllocLeaf(BPlusTree tree, leaf_t *node)
{
    off_t slot;
    node->n = 0;
    tree->meta.leafNum++;
    slot = tree->meta.slot;
    return slot;
}

off_t AllocInternal(BPlusTree tree, internal_t *node)
{
    off_t slot;
    node->n = 1;
    tree->meta.internalNum++;
    slot = tree->meta.slot;
    tree->meta.slot += sizeof(internal_t);
    return slot;
}

off_t AllocSize(BPlusTree tree, size_t size)
{
    off_t slot;
    slot = tree->meta.slot;
    tree->meta.slot += size;
    return slot;
}

// Insert
int Insert(BPlusTree tree, my_key_t key, value_t value)
{
    off_t parent, offset;
    leaf_t leaf, newLeaf;
    internal_t tmpInternal;
    record_t tmpRecord, swapRecord;
    off_t newOffset;
    int i, j, leafChildrenNum, compareRes;
    parent = SearchIndex(tree, key);
    offset = SearchLeaf(tree, parent, key);
    ReadIndexBlock(tree, &leaf, offset, sizeof(leaf_t));
    leafChildrenNum = leaf.n;
    tmpRecord.key = key;
    tmpRecord.value = value;
    // check whether in the tree first
    for (i = 0; i < leafChildrenNum; i++)
    {
        compareRes = KeyCmp(leaf.children[i + 1].key, tmpRecord.key);
        if (compareRes == 0)  // the key is already in the tree
        {
            return 1;
        }
        if (compareRes > 0)
        {
            break;
        }
    }
    // case 1: no need to split
    if (leaf.n < tree->meta.order)
    {
        // do insertion sort
        InsertIntoLeaf(&leaf, &tmpRecord);
        // write back
        WriteIndexBlock(tree, &leaf, offset, sizeof(leaf_t));
    }
    // case 2: Oops, we have to split the tree
    else  // leaf.n == tree->meta.order
    {
        newOffset = AllocLeaf(tree, &newLeaf);
        newLeaf.parent = leaf.parent;
        newLeaf.next = leaf.next;
        leaf.next = newOffset;
        newLeaf.prev = offset;
        ReadIndexBlock(tree, &tmpInternal, offset, sizeof(internal_t));
        if (tmpInternal.n < tree->meta.order)  // case 2.1, new key in parent fits
        {
            // let swapRecord be the largest one
            if (KeyCmp(leaf.children[leaf.n].key, tmpRecord.key) > 0)
            {
                // swap
                swapRecord = leaf.children[leaf.n];
                leaf.n--;
                InsertIntoLeaf(&leaf, &tmpRecord);
            }
            else  // tmpRecord is the largest one
            {
                swapRecord = tmpRecord;
            }
            // copy the right half of children of leaf to new leaf
            for (i = (int)leaf.n / 2, j = 0; i < (int)leaf.n; i++, j++)
            {
                newLeaf.children[j] = leaf.children[i];
            }
            newLeaf.children[j] = swapRecord;
            newLeaf.n = j + 1;
            leaf.n = leaf.n / 2;
            WriteIndexBlock(tree, &leaf, offset, sizeof(leaf_t));
            WriteIndexBlock(tree, &newLeaf, offset, sizeof(leaf_t));
            // @TODO we need to
            while (1)
            {

            }
        }

    }
    return 0;
}

int SearchIndex(BPlusTree tree, my_key_t key)
{

    return 0;
}

int SearchLeaf(BPlusTree tree, off_t parent, my_key_t key)
{

    return 0;
}

int KeyCmp(my_key_t A, my_key_t B)
{
    return strcmp(A.key, B.key);
}

void InsertIntoLeaf(leaf_t *leaf, record_t *newRecord)
{
    int i;
    i = leaf->n;
    while (i >= 0 && KeyCmp(leaf->children[i].key, newRecord->key) > 0)
    {
        leaf->children[i + 1] = leaf->children[i];
        i--;
    }
    leaf->children[i + 1] = *newRecord;
    leaf->n++;
}
