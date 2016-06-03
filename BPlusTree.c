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
#ifdef NOBUFFER
    free(leaf);
#endif
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
#ifdef NOBUFFER
        free(node);
#endif
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
#ifdef NOBUFFER
        free(child);
#endif
    }
}

int Remove(BPlusTree tree, my_key_t key)
{
    internal_t *parent;
    leaf_t *leaf, *sibling;
    off_t offset, parentOffset;
    size_t minChildrenNum;
    my_key_t oldKey;
    int i, compareRes, keyPos, done = 0;
    // First search the key we are going to remove
    parentOffset = SearchIndex(tree, key);
    offset = SearchLeaf(tree, parentOffset, key);
    leaf = (leaf_t *)ReadBlock(tree->path, offset, sizeof(leaf_t));
    for (i = 0; i < (int)leaf->n; i++)
    {
        compareRes = KeyCmp(leaf->children[i].key, key);
        if (compareRes == 0)
        {
            keyPos = i;
            break;
        }
    }
    if (i == (int)leaf->n)  // not found
    {
        return 1;
    }

    minChildrenNum = tree->meta.leafNum == 1 ? 0 : tree->meta.order / 2;
    // delete the key, move keys behind it one by one
    for (i = keyPos; i < (int)leaf->n - 1; i++)
    {
        leaf->children[i] = leaf->children[i + 1];
    }
    leaf->n--;
    if (leaf->n < minChildrenNum)  // Oops, we have to merge the leaf with a neighbor or borrow a child from its sibling
    {
        if (leaf->prev)  // try to borrow from left
        {
            done = BorrowKey(tree, 0, leaf);
        }
        if (!done && leaf->next)  // try to borrow from right
        {
            done = BorrowKey(tree, 1, leaf);
        }
        // try to merge
        if (!done)
        {
            parent = (internal_t *)ReadBlock(tree->path, parentOffset, sizeof(internal_t));
            if (parent->children[parent->n - 1].child == offset)  // leaf is the last child of parent, merge it with leaf->prev
            {
                sibling = (leaf_t *)ReadBlock(tree->path, leaf->prev, sizeof(leaf_t));
                MergeLeaves(sibling, leaf);
                RemoveLeaf(tree, sibling, leaf);
                WriteBlock(tree->path, sibling, leaf->prev, sizeof(leaf_t));
#ifdef NOBUFFER
                free(sibling);
#endif
                oldKey = leaf->children[0].key;
            }
            else  // merge leaf with leaf->next
            {
                sibling = (leaf_t *)ReadBlock(tree->path, leaf->next, sizeof(leaf_t));
                MergeLeaves(leaf, sibling);
                RemoveLeaf(tree, leaf, sibling);
                WriteBlock(tree->path, leaf, offset, sizeof(leaf_t));
                oldKey = sibling->children[0].key;
#ifdef NOBUFFER
                free(sibling);
#endif
            }
            RemoveIndex(tree, parent, parentOffset, oldKey);
        }
        else
        {  // succeed borrowing, write back leaf
            WriteBlock(tree->path, leaf, offset, sizeof(leaf_t));
        }
    }
    else  // no need to borrow or merge
    {
        WriteBlock(tree->path, leaf, offset, sizeof(leaf_t));
    }
#ifdef NOBUFFER
    free(leaf);
#endif
    return 0;
}

int BorrowKey(BPlusTree tree, int borrowFromRight, leaf_t *leaf)
{
    off_t siblingOffset;
    leaf_t *sibling;
    int i, borrowPos, storePos;

    siblingOffset = borrowFromRight ? leaf->next : leaf->prev;
    sibling = (leaf_t *)ReadBlock(tree->path, siblingOffset, sizeof(leaf_t));
    if (sibling->n <= tree->meta.order / 2)  // cannot borrow
    {
        return 0;
    }
    if (borrowFromRight)
    {
        borrowPos = 0;
        storePos = leaf->n;
        UpdateIndexChild(tree, sibling->parent, sibling->children[0].key, sibling->children[1].key);
    }
    else
    {
        borrowPos = sibling->n - 1;
        storePos = 0;
        UpdateIndexChild(tree, leaf->parent, leaf->children[0].key, sibling->children[borrowPos].key);
    }
    // insert into leaf
    for (i = (int)leaf->n - 1; i >= storePos; i--)  // shift leaf->children[i] right
    {
        leaf->children[i + 1] = leaf->children[i];
    }
    leaf->children[storePos] = sibling->children[borrowPos];
    leaf->n++;
    // remove from sibling
    for (i = borrowPos + 1; i < (int)sibling->n; i++)
    {
        sibling->children[i - 1] = sibling->children[i];
    }
    sibling->n--;
    WriteBlock(tree->path, sibling, siblingOffset, sizeof(leaf_t));
#ifdef NOBUFFER
        free(sibling);
#endif
    return 1;  // succeeded
}

void UpdateIndexChild(BPlusTree tree, off_t parentOffset, my_key_t oldKey, my_key_t newKey)
{
    internal_t *parent;
    int i;
    parent = (internal_t *)ReadBlock(tree->path, parentOffset, sizeof(internal_t));
    for (i = 0; i < (int)parent->n; i++)
    {
        if (KeyCmp(parent->children[i].key, oldKey) >= 0)  // found old key
        {
            parent->children[i].key = newKey;
            break;
        }
    }
    if (0 == i && 0 != parent->parent)
    {
        UpdateIndexChild(tree, parent->parent, oldKey, newKey);
    }
    WriteBlock(tree->path, parent, parentOffset, sizeof(internal_t));
#ifdef NOBUFFER
    free(parent);
#endif
}

int MergeLeaves(leaf_t *left, leaf_t *right)
{
    int i, j;
    for (i = 0, j = (int)left->n; i < (int)right->n; i++, j++)
    {
        left->children[j] = right->children[i];
    }
    left->n += right->n;
    return 0;
}

int RemoveLeaf(BPlusTree tree, leaf_t *left, leaf_t *right)
{
    leaf_t *rightNext;
    left->next = right->next;
    UnallocLeaf(tree);
    if (right->next)
    {
        rightNext = (leaf_t *)ReadBlock(tree->path, right->next, sizeof(leaf_t));
        rightNext->prev = right->prev;
        WriteBlock(tree->path, rightNext, right->next, sizeof(leaf_t));
    }
    WriteBlock(tree->path, &tree->meta, META_OFFSET, BLOCK_SIZE);
    return 0;
}

int RemoveIndex(BPlusTree tree, internal_t *node, off_t offset, my_key_t oldKey)
{
    size_t minChildrenNum;
    internal_t *parent, *sibling;
    my_key_t keytoRemove;
    int i, keyPos, compareRes, done = 0;
    for(i = 0; i < (int)node->n; i++)
    {
        compareRes = KeyCmp(node->children[i].key, oldKey);
        if (compareRes == 0)
        {
            keyPos = i;
            break;
        }
    }
    minChildrenNum = node->parent == 0 ? 1 : tree->meta.order / 2;
    // delete the key, move keys behind it one by one
    for (i = keyPos; i < (int)node->n - 1; i++)
    {
        node->children[i] = node->children[i + 1];
    }
    node->n--;
    // if node is root and has one key now
    if (0 == node->parent && 1 == node->n && tree->meta.leafNum != 1)
    {
        UnallocInternal(tree);
        tree->meta.rootOffset = node->children[0].child;
        tree->meta.height--;
        WriteBlock(tree->path, &tree->meta, META_OFFSET, BLOCK_SIZE);
#ifdef NOBUFFER
        free(node);
#endif
        return 0;
    }

    if (node->n < minChildrenNum)  // Oops, we have to merge the node with a neighbor or borrow a child from its sibling
    {
        // @TODO bug here: node->prev == node->next == 0, but it should not be that case
        if (node->prev)  // try to borrow from left
        {
            done = BorrowKeyFromInternal(tree, 0, node, offset);
        }
        if (!done && node->next)  // try to borrow from right
        {
            done = BorrowKeyFromInternal(tree, 1, node, offset);
        }
        // try to merge
        if (!done)
        {
            parent = (internal_t *)ReadBlock(tree->path, node->parent, sizeof(internal_t));
            if (parent->children[parent->n - 1].child == offset)  // ndoe is the last child of parent, merge it with node->prev
            {
                sibling = (internal_t *)ReadBlock(tree->path, node->prev, sizeof(internal_t));
                MergeInternals(sibling, node);
                RemoveInternal(tree, sibling, node);
                WriteBlock(tree->path, sibling, node->prev, sizeof(internal_t));
#ifdef NOBUFFER
                free(sibling);
#endif
                keytoRemove = node->children[0].key;
            }
            else  // merge node with node->next
            {
                sibling = (internal_t *)ReadBlock(tree->path, node->next, sizeof(internal_t));
                MergeInternals(node, sibling);
                RemoveInternal(tree, node, sibling);
                WriteBlock(tree->path, node, offset, sizeof(internal_t));
#ifdef NOBUFFER
                free(sibling);
#endif
                keytoRemove = sibling->children[0].key;
            }
            RemoveIndex(tree, parent, node->parent, keytoRemove);
        }
        else  // succeed borrowing, write back leaf
        {
            WriteBlock(tree->path, node, offset, sizeof(internal_t));
        }
    }
    else   // no need to borrow or merge
    {
            WriteBlock(tree->path, node, offset, sizeof(internal_t));
    }
#ifdef NOBUFFER
    free(node);
#endif
    return 0;
}

void UnallocLeaf(BPlusTree tree)
{
    tree->meta.leafNum--;
}

void UnallocInternal(BPlusTree tree)
{
    tree->meta.internalNum--;
}

int BorrowKeyFromInternal(BPlusTree tree, int borrowFromRight, internal_t *node, off_t offset)
{

    return 0;
}

void MergeInternals(internal_t *left, internal_t *right)
{

}

void RemoveInternal(BPlusTree tree, internal_t *left, internal_t *right)
{

}
