#include <string.h>
#include "BPlusTree.h"
#include "MiniSQL.h"
// ======================= buffer read and write =======================
// ReadBlock and WriteBlock functions are provided by Buffer module
#ifdef NOBUFFER
void *ReadBlock(char *fileName, off_t offset, size_t size)
{
    void *block;
    FILE *fp;
    int n;
    block = (void *)malloc(size);  // @NOTE don't forget to free space later
    fp = fopen(fileName, "rb+");
    fseek(fp, offset, SEEK_SET);
    n = fread(block, size, 1, fp);  // @NOTE be careful because it must read Successfully
    if (n != 1)
    {
        printf("Failed to read block: read %d block(s), offset = %ld\n", n, offset);
        return NULL;
    }
    fclose(fp);
    return block;
}

int WriteBlock(char *fileName, void *block, off_t offset, size_t size)
{
    FILE *fp;
    int ret;
    fp = fopen(fileName, "rb+");
    fseek(fp, offset, SEEK_SET);
    ret = fwrite(block, BLOCK_SIZE, 1, fp);
    fclose(fp);
    return ret;
}
#endif
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
void InitTree(BPlusTree tree, char *path, enum DataType type)
{
    internal_t root;
    leaf_t leaf;
    // initialize meta data
    strcpy(tree->path, path);
    tree->meta.order = TREE_ORDER;
    tree->meta.valueSize = sizeof(value_t);
    tree->meta.keySize = sizeof(my_key_t);
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
    tree->meta.type = type;
    // write back to buffer
    WriteBlock(tree->path, &tree->meta, META_OFFSET, BLOCK_SIZE);
    WriteBlock(tree->path, &root, tree->meta.rootOffset, BLOCK_SIZE);
    WriteBlock(tree->path, &leaf, tree->meta.leafOffset, BLOCK_SIZE);
}

off_t AllocLeaf(BPlusTree tree, leaf_t *node)
{
    off_t slot;
    node->n = 0;
    tree->meta.leafNum++;
    slot = tree->meta.slot;
    tree->meta.slot += BLOCK_SIZE;
#ifndef DEBUG
    printf("slot: %ld\n", slot);
#endif
    return slot;
}

off_t AllocInternal(BPlusTree tree, internal_t *node)
{
    off_t slot;
    node->n = 1;
    tree->meta.internalNum++;
    slot = tree->meta.slot;
    tree->meta.slot += BLOCK_SIZE;
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
    internal_t *tmpInternal, newInternal, root;
    record_t tmpRecord;
    off_t newLeafOffset, tmpInternalOffset, newInternalOffset, newRootOffset;
    index_t newIndex, firstChild;
    int height;

    parent = SearchIndex(tree, key);
    offset = SearchLeaf(tree, parent, key);
    leaf = (leaf_t *)ReadBlock(tree->path, offset, sizeof(leaf_t));
    tmpRecord.key = key;
    tmpRecord.value = value;
    // check whether in the tree first
    if (SearchKeyInLeaf(key, leaf))
    {
        printf("The key(%d) is already in the table\n", key.key);
        return 1;
    }
    // case 1: no need to split
    if (leaf->n < tree->meta.order)
    {
        // do insertion sort
        InsertIntoLeaf(leaf, &tmpRecord);
        // write back
        WriteBlock(tree->path, leaf, offset, sizeof(leaf_t));
#ifdef NOBUFFER
        free(leaf);
#endif
    }
    // case 2: Oops, we have to split the tree
    else  // leaf.n == tree->meta.order
    {
        // create new leaf
        newLeafOffset = CreateNewLeaf(tree, leaf, offset, &newLeaf);
        tmpInternal = (internal_t *)ReadBlock(tree->path, leaf->parent, sizeof(internal_t));  // read parent
        // copy the right half of a full leaf to a new leaf node
        CopyLeaf(leaf, &newLeaf, tmpRecord);
        // after copy, we may have to reset the index of the old leaf ndoe
        // @NOTE patch works
        if (0 == KeyCmp(leaf->children[0].key, tmpRecord.key))
        {
            //ResetIndex(tree, leaf, key);
            tmpInternal->children[0].key = tmpRecord.key;
            tmpInternal->children[0].child = offset;
            ResetIndexR(tree, tmpInternal, tmpRecord.key, leaf->parent);
        }
        WriteBlock(tree->path, leaf, offset, sizeof(leaf_t));
        WriteBlock(tree->path, &newLeaf, newLeafOffset, sizeof(leaf_t));
        // split recursively
        // we need to insert newIndex into tmpInternal
        newIndex.key = newLeaf.children[0].key;
        newIndex.child = newLeafOffset;
        height = tree->meta.height - 1;  // the current level we are dealing with
        tmpInternalOffset = leaf->parent;
        firstChild.key = leaf->children[0].key;
        firstChild.child = offset;
        if (1 == tmpInternal->n)  // insert the smallest one
        {
            tmpInternal->children[0].key = leaf->children[0].key;
            tmpInternal->children[0].child = offset;
        }
#ifdef NOBUFFER
        free(leaf);
#endif
        while (height >= 0)
        {
            // fits in internal node, don't split
            if (tmpInternal->n < tree->meta.order)
            {
                InsertIntoInternal(tmpInternal, newIndex);
                WriteBlock(tree->path, tmpInternal, tmpInternalOffset, sizeof(internal_t));
                break;
            }
            // @NOTE Important here, reset the parent's first child because it might be changed if a smaller node was inserted
            if (KeyCmp(tmpInternal->children[0].key, firstChild.key) > 0)
            {
                tmpInternal->children[0].key = firstChild.key;
            }
            // we should create a root first
            if (0 == tmpInternal->parent)  // we reach the root node and need a new root
            {
                newRootOffset = CreateNewRoot(tree, &root, tmpInternal, tmpInternalOffset);
                tmpInternal->parent = newRootOffset;
                height++;
            }
            // full, split it
            newInternalOffset = AllocInternal(tree, &newInternal);
            newInternal.parent = tmpInternal->parent;
            newInternal.next = tmpInternal->next;
            newInternal.prev = tmpInternalOffset;
            CopyInternal(&newIndex, tmpInternal, &newInternal);
            if (0 == KeyCmp(tmpInternal->children[0].key, newIndex.key))
            {
                ResetIndexR(tree, tmpInternal, newIndex.key, tmpInternalOffset);
            }
            WriteBlock(tree->path, tmpInternal, tmpInternalOffset, sizeof(internal_t));
            WriteBlock(tree->path, &newInternal, newInternalOffset, sizeof(internal_t));
            // after CopyInternal, we have to reset children's parent
            ResetIndexParent(tree, &newInternal, newInternalOffset);  // reset the children's parent as the newInternal node
            // move to the upper level
            // update newIndex
            newIndex.key = newInternal.children[0].key;
            newIndex.child = newInternalOffset;
            tmpInternalOffset = tmpInternal->parent;
#ifdef NOBUFFER
            free(tmpInternal);
#endif
            tmpInternal = (internal_t *)ReadBlock(tree->path, tmpInternalOffset, sizeof(internal_t));
            height--;
        }
#ifdef NOBUFFER
        free(tmpInternal);
#endif
    }  // case 2: split tree
    WriteBlock(tree->path, &tree->meta, META_OFFSET, sizeof(meta_t));  // update meta data
    return 0;
}

off_t SearchIndex(BPlusTree tree, my_key_t key)
{
    off_t offset = tree->meta.rootOffset;
    int i, height = tree->meta.height;
    internal_t *node;
    //index_t *index;
    while (height > 1)
    {
        node = (internal_t *)ReadBlock(tree->path, offset, sizeof(internal_t));
        for (i = (int)node->n - 1; i >= 0; i--)  // ignore the value of node->children[0]
        {
            //index = &node->children[i];
            offset = node->children[i].child;
            if (KeyCmp(node->children[i].key, key) <= 0)
            {
                break;
            }
        }
        //offset = index->child;
#ifdef NOBUFFER
        free(node);
#endif
        height--;
    }
    return offset;
}

off_t SearchLeaf(BPlusTree tree, off_t parent, my_key_t key)
{
    off_t offset;
    internal_t *node;
    int i;
    node = (internal_t *)ReadBlock(tree->path, parent, sizeof(internal_t));
    for (i = (int)node->n - 1; i >= 0; i--)
    {
        offset = node->children[i].child;
        if (1 == tree->meta.leafNum || KeyCmp(node->children[i].key, key) <= 0)
        {
            break;
        }
    }
#ifdef NOBUFFER
    free(node);
#endif
    return offset;
}

int KeyCmp(my_key_t A, my_key_t B)
{
    return KeyValueCmp(A.key, B.key);
}

int IntKeyCmp(int A, int B)
{
    return A - B;
}
int FloatKeyCmp(float  A, float B)
{
    if (A < B)
    {
        return -1;
    }
    else if (A == B)
    {
        return 0;
    }
    else return 1;
}

int StringKeyCmp(char *A, char *B)
{
    return strcmp(A, B);
}

void InsertIntoLeaf(leaf_t *leaf, record_t *newRecord)
{
    int i;
    i = leaf->n - 1;
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
    i = internal->n - 1;
    while (i >= 0 && KeyCmp(internal->children[i].key, index.key) > 0)
    {
        internal->children[i + 1] = internal->children[i];
        i--;
    }
    internal->children[i + 1] = index;
    internal->n++;
#ifndef DEBUG
    printf("After insert %d, parent: %ld\n", index.key.key, internal->parent);
    for (i = 0; i < (int)internal->n; i++)
    {
        printf("internal->children[%d]: %d, %ld\n", i, internal->children[i].key.key, internal->children[i].child);
    }
#endif
}


value_t Search(BPlusTree tree, my_key_t key)
{
    off_t parent, offset;
    leaf_t *leaf;
    int i, compareRes;
    parent = SearchIndex(tree, key);
    offset = SearchLeaf(tree, parent, key);
    leaf = (leaf_t *)ReadBlock(tree->path, offset, sizeof(leaf_t));
    for (i = 0; i < (int)leaf->n; i++)
    {
        compareRes = KeyCmp(leaf->children[i].key, key);
        if (compareRes == 0)
        {
            return leaf->children[i].value;
        }
    }
    if (i == (int)leaf->n)
    {
        printf("The key is not in the BPlusTree\n");
    }
    return 0;
}

int SearchKeyInLeaf(my_key_t key, leaf_t *leaf)
{
    int i, compareRes;
    for (i = 0; i < (int)leaf->n; i++)
    {
        compareRes = KeyCmp(leaf->children[i].key, key);
        if (compareRes == 0)  // the key is already in the tree
        {
            return 1;  // found
        }
        if (compareRes > 0)  // compareRes > 0 means that the key does not exists because the keys in a node are in ascending order
        {
            break;
        }
    }
    return 0;  // not found
}

// @TODO after copy, what should be updated in the parent node
void CopyLeaf(leaf_t *leaf, leaf_t *newLeaf, record_t tmpRecord)  // copy the right half of a full leaf to a new leaf node
{
    int i, j;
    record_t swapRecord;
    if (KeyCmp(leaf->children[leaf->n - 1].key, tmpRecord.key) > 0)
    {
        // swap
        swapRecord = leaf->children[leaf->n - 1];
        leaf->n--;
        InsertIntoLeaf(leaf, &tmpRecord);
    }
    else  // tmpRecord is the largest one
    {
        swapRecord = tmpRecord;
    }
    // copy the right half of children of leaf to new leaf
    for (i = (int)leaf->n / 2 + 1, j = 0; i < (int)leaf->n; i++, j++)
    {
        newLeaf->children[j] = leaf->children[i];
    }
    newLeaf->children[j] = swapRecord;
    if (leaf->n % 2)
    {
        newLeaf->n = leaf->n / 2 + 1;
    }
    else
    {
        newLeaf->n = leaf->n / 2;
    }
    leaf->n += 1 - newLeaf->n;
}


void ResetIndexR(BPlusTree tree, internal_t *tmpInternal, my_key_t key, off_t offset)
{
    int i;
    index_t newIndex;
    off_t parent;
    internal_t *node;
    newIndex.key = key;
    newIndex.child = offset;
    parent = tmpInternal->parent;
    while (parent)
    {
        node = (internal_t *)ReadBlock(tree->path, parent, sizeof(internal_t));
        for (i = 0; i < (int)node->n; i++)
        {
            if (node->children[i].child == newIndex.child)
            {
                node->children[i] = newIndex;
                break;
            }
        }
        WriteBlock(tree->path, node, parent, sizeof(internal_t));
        parent = node->parent;
    }
}

void CopyInternal(index_t *newIndex, internal_t *tmpInternal, internal_t *newInternal)
{
    int i, j;
    index_t swapIndex;
    if (KeyCmp(newIndex->key, tmpInternal->children[tmpInternal->n - 1].key) > 0)
    {
        swapIndex = *newIndex;
    }
    else
    {
        swapIndex = tmpInternal->children[tmpInternal->n - 1];
        tmpInternal->n--;
        InsertIntoInternal(tmpInternal, *newIndex);
    }
    for (i = (int)tmpInternal->n / 2 + 1, j = 0; i < (int)tmpInternal->n; i++, j++)
    {
        // copy the right half to the new internal node
        newInternal->children[j] = tmpInternal->children[i];
    }
    newInternal->children[j] = swapIndex;
    if (tmpInternal->n % 2)
    {
        newInternal->n = tmpInternal->n / 2 + 1;
    }
    else
    {
        newInternal->n = tmpInternal->n / 2;
    }
    tmpInternal->n += 1 - newInternal->n;
}

off_t CreateNewLeaf(BPlusTree tree, leaf_t *leaf, off_t offset, leaf_t *newLeaf)
{
    off_t newLeafOffset;
    newLeafOffset = AllocLeaf(tree, newLeaf);
    newLeaf->parent = leaf->parent;
    newLeaf->next = leaf->next;
    leaf->next = newLeafOffset;
    newLeaf->prev = offset;
    return newLeafOffset;
}

off_t CreateNewRoot(BPlusTree tree, internal_t *root, internal_t *tmpInternal, off_t tmpInternalOffset)
{
    off_t newRootOffset;
    newRootOffset = AllocInternal(tree, root);
    tree->meta.rootOffset = newRootOffset;
    tree->meta.height++;
    root->prev = root->next = root->parent = 0;
    // update children of root
    root->children[0].key = tmpInternal->children[0].key;
    root->children[0].child = tmpInternalOffset;
#ifndef DEBUG
    printf("root->children[1]: %d, %ld\n", newInternal.children[1].key.key, newInternal.children[1].child);
#endif
    // update root->n
    root->n = 1;
    WriteBlock(tree->path, root, newRootOffset, sizeof(internal_t));
    return newRootOffset;
}

void ResetIndexParent(BPlusTree tree, internal_t *internal, off_t offset)  // reset the children's parent as the newInternal node
{
    int i;
    internal_t *child;
    off_t childOffset;
    for (i = 0; i < (int)internal->n; i++)
    {
        childOffset = internal->children[i].child;
        child = (internal_t *)ReadBlock(tree->path, childOffset, sizeof(internal_t));
        child->parent = offset;
        WriteBlock(tree->path, child, childOffset, BLOCK_SIZE);
        free(child);
    }
}
