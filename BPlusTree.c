#include <string.h>
#include "BPlusTree.h"
// ======================= buffer read and write =======================
// ReadBlock and WriteBlock functions are provided by Buffer module
void *ReadBlock(char *fileName, off_t offset, size_t size)
{
    void *block;
    FILE *fp;
    block = (void *)malloc(size);  // @NOTE don't forget to free space later
    fp = fopen(fileName, "rb+");
    fseek(fp, offset, SEEK_SET);
    fread(block, size, 1, fp);  // @NOTE be careful because it must read Successfully
    fclose(fp);
    return block;
}

int WriteBlock(char *fileName, void *block, off_t offset, size_t size)
{
    FILE *fp;
    int ret;
    fp = fopen(fileName, "rb+");
    fseek(fp, offset, SEEK_SET);
    ret = fwrite(block, size, 1, fp);
    fclose(fp);
    return ret;
}
// ========================= not my work above =========================
// This is my part
/* not use any more
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
    unsigned int isDone;  // Successful or not
    isDone = (ReadBlock(tree->path, offset, size) == NULL);
    return isDone;
}

int WriteIndexBlock(BPlusTree tree, void *block, off_t offset, size_t size)
{
    int isDone;
    OpenFile(tree);
    isDone = (WriteBlock(tree->path, block, offset, size) == 0);
    CloseFile(tree);
    return isDone;
}
*/
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
    WriteBlock(tree->path, &tree->meta, META_OFFSET, sizeof(meta_t));
    WriteBlock(tree->path, &root, tree->meta.rootOffset, sizeof(internal_t));
    WriteBlock(tree->path, &leaf, tree->meta.leafOffset, sizeof(leaf_t));
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
    leaf_t *leaf, newLeaf;
    internal_t *tmpInternal, newInternal;
    record_t tmpRecord, swapRecord;
    off_t newLeafOffset, tmpInternalOffset, newInternalOffset;
    index_t newIndex, swapIndex;
    int i, j, leafChildrenNum, compareRes, height;
    parent = SearchIndex(tree, key);
    offset = SearchLeaf(tree, parent, key);
    leaf = (leaf_t *)ReadBlock(tree->path, offset, sizeof(leaf_t));
    leafChildrenNum = leaf->n;
    tmpRecord.key = key;
    tmpRecord.value = value;
    // check whether in the tree first
    for (i = 0; i < leafChildrenNum; i++)
    {
        compareRes = KeyCmp(leaf->children[i + 1].key, tmpRecord.key);
        if (compareRes == 0)  // the key is already in the tree
        {
            return 1;
        }
        if (compareRes > 0)  // compareRes > 0 means that the key does not exists because the keys in a node are in ascending order
        {
            break;
        }
    }
    // case 1: no need to split
    if (leaf->n < tree->meta.order)
    {
        // do insertion sort
        InsertIntoLeaf(leaf, &tmpRecord);
        // write back
        WriteBlock(tree->path, &leaf, offset, sizeof(leaf_t));
    }
    // case 2: Oops, we have to split the tree
    else  // leaf.n == tree->meta.order
    {
        newLeafOffset = AllocLeaf(tree, &newLeaf);
        newLeaf.parent = leaf->parent;
        newLeaf.next = leaf->next;
        leaf->next = newLeafOffset;
        newLeaf.prev = offset;
        tmpInternal = (internal_t *)ReadBlock(tree->path, leaf->parent, sizeof(internal_t));  // read parent
        if (tmpInternal->n < tree->meta.order)  // case 2.1, new key in parent fits
        {
            // let swapRecord be the largest one
            if (KeyCmp(leaf->children[leaf->n].key, tmpRecord.key) > 0)
            {
                // swap
                swapRecord = leaf->children[leaf->n];
                leaf->n--;
                InsertIntoLeaf(leaf, &tmpRecord);
            }
            else  // tmpRecord is the largest one
            {
                swapRecord = tmpRecord;
            }
            // copy the right half of children of leaf to new leaf
            for (i = (int)leaf->n / 2, j = 0; i < (int)leaf->n; i++, j++)
            {
                newLeaf.children[j] = leaf->children[i];
            }
            newLeaf.children[j] = swapRecord;
            newLeaf.n = leaf->n / 2 + 1;
            leaf->n += 1 - newLeaf.n;
            WriteBlock(tree->path, &leaf, offset, sizeof(leaf_t));
            WriteBlock(tree->path, &newLeaf, newLeafOffset, sizeof(leaf_t));
            // @TODO split recursively
            // we need to insert newIndex into tmpInternal
            newIndex.key = newLeaf.children[0].key;
            newIndex.child = newLeafOffset;
            height = tree->meta.height;
            tmpInternalOffset = leaf->parent;
            while (height > 0)
            {
                // fits, don't split
                if (tmpInternal->n < tree->meta.order)
                {
                    InsertIntoInternal(tmpInternal, newIndex);
                    WriteBlock(tree->path, tmpInternal, tmpInternalOffset, sizeof(internal_t));
                    break;
                }
                newInternalOffset = AllocInternal(tree, &newInternal);
                newInternal.parent = tmpInternal->parent;
                newInternal.next = tmpInternal->next;
                newInternal.prev = tmpInternalOffset;
                // set swapIndex = the largest index
                if (KeyCmp(newIndex.key, tmpInternal->children[tmpInternal->n].key) > 0)
                {
                    swapIndex = newIndex;
                }
                else
                {
                    swapIndex = tmpInternal->children[tmpInternal->n];
                    tmpInternal->n--;
                    InsertIntoInternal(tmpInternal, newIndex);
                }
                for (i = (int)tmpInternal->n / 2, j = 0; i < (int)tmpInternal->n; i++, j++)
                {
                    // copy the right half to the new internal node
                    newInternal.children[j] = tmpInternal->children[i];
                }
                newInternal.children[j] = swapIndex;
                newInternal.n = tmpInternal->n / 2 + 1;
                tmpInternal->n += 1 - newInternal.n;
                WriteBlock(tree->path, tmpInternal, tmpInternalOffset, sizeof(internal_t));
                WriteBlock(tree->path, &newInternal, newInternalOffset, sizeof(internal_t));
                // move to the upper level
                newIndex = newInternal.children[0];
                tmpInternalOffset = tmpInternal->parent;
                tmpInternal = (internal_t *)ReadBlock(tree->path, tmpInternalOffset, sizeof(internal_t));
                height--;
            }
            // @TODO insert not yet done orz
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

void InsertIntoInternal(internal_t *internal, index_t index)
{
    int i;
    i = internal->n;
    while (i >= 0 && KeyCmp(internal->children[i].key, index.key) > 0)
    {
        internal[i + 1] = internal[i];
        i--;
    }
    internal->children[i + 1] = index;
    internal->n++;
}
