#include <string.h>
#include "BPlusTree.h"
#include "BPlusTreeInt.h"
#include "../MiniSQL.h"
// initialize tree
off_t AllocLeaf_int(BPlusTree tree, leaf_t_int *node)
{
    off_t slot;
    node->n = 0;
    tree->meta.leafNum++;
    slot = tree->meta.slot;
    tree->meta.slot += BLOCK_SIZE;
    return slot;
}

off_t AllocInternal_int(BPlusTree tree, internal_t_int *node)
{
    off_t slot;
    node->n = 1;
    tree->meta.internalNum++;
    slot = tree->meta.slot;
    tree->meta.slot += BLOCK_SIZE;
    return slot;
}

void InitTree_int(BPlusTree tree, char *path, enum DataType type)
{
    internal_t_int root;
    leaf_t_int leaf;
    // initialize meta data
    strcpy(tree->path, path);
    tree->meta.order = TREE_ORDER_int;
    tree->meta.valueSize = sizeof(value_t);
    tree->meta.keySize = sizeof(my_key_t_int);
    tree->meta.internalNum = 0;
    tree->meta.leafNum = 0;
    tree->meta.height = 1;
    tree->meta.slot = BLOCK_OFFSET;
    // initialize root
    root.next = root.prev = root.parent = 0;
    tree->meta.rootOffset = AllocInternal_int(tree, &root);
    // initialize leaf
    leaf.next = leaf.prev = 0;
    leaf.parent = tree->meta.rootOffset;
    tree->meta.leafOffset = root.children[0].child = AllocLeaf_int(tree, &leaf);
    tree->meta.type = type;
    // write back to buffer
    WriteBlock(tree->path, &tree->meta, META_OFFSET, BLOCK_SIZE);
    WriteBlock(tree->path, &root, tree->meta.rootOffset, BLOCK_SIZE);
    WriteBlock(tree->path, &leaf, tree->meta.leafOffset, BLOCK_SIZE);
}

// Insert_int
int Insert_int(BPlusTree tree, my_key_t_int key, value_t value)
{
    off_t parent, offset;
    leaf_t_int *leaf, newLeaf;
    internal_t_int *tmpInternal, newInternal, root;
    record_t_int tmpRecord;
    off_t newLeafOffset, tmpInternalOffset, newInternalOffset, newRootOffset;
    index_t_int newIndex, firstChild;
    int height;

    parent = SearchIndex_int(tree, key);
    offset = SearchLeaf_int(tree, parent, key);
    leaf = (leaf_t_int *)ReadBlock(tree->path, offset, sizeof(leaf_t_int));
    tmpRecord.key = key;
    tmpRecord.value = value;
    // check whether in the tree first
    if (SearchKeyInLeaf_int(key, leaf))
    {
        return 1;
    }
    // case 1: no need to split
    if (leaf->n < tree->meta.order)
    {
        // do insertion sort
        InsertIntoLeaf_int(leaf, &tmpRecord);
        // write back
        WriteBlock(tree->path, leaf, offset, sizeof(leaf_t_int));
#ifdef NOBUFFER
        free(leaf);
#endif
    }
    // case 2: Oops, we have to split the tree
    else  // leaf.n == tree->meta.order
    {
        // create new leaf
        newLeafOffset = CreateNewLeaf_int(tree, leaf, offset, &newLeaf);
        tmpInternal = (internal_t_int *)ReadBlock(tree->path, leaf->parent, sizeof(internal_t_int));  // read parent
        // copy the right half of a full leaf to a new leaf node
        CopyLeaf_int(leaf, &newLeaf, tmpRecord);
        // after copy, we may have to reset the index of the old leaf ndoe
        // @NOTE patch works
        if (0 == KeyCmp_int(leaf->children[0].key, tmpRecord.key))
        {
            tmpInternal->children[0].key = tmpRecord.key;
            tmpInternal->children[0].child = offset;
            ResetIndexR_int(tree, tmpInternal, tmpRecord.key, leaf->parent);
        }
        WriteBlock(tree->path, leaf, offset, sizeof(leaf_t_int));
        WriteBlock(tree->path, &newLeaf, newLeafOffset, sizeof(leaf_t_int));
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
                InsertIntoInternal_int(tmpInternal, newIndex);
                WriteBlock(tree->path, tmpInternal, tmpInternalOffset, sizeof(internal_t_int));
                break;
            }
            // @NOTE Important here, reset the parent's first child because it might be changed if a smaller node was inserted
            if (KeyCmp_int(tmpInternal->children[0].key, firstChild.key) > 0)
            {
                tmpInternal->children[0].key = firstChild.key;
            }
            // we should create a root first
            if (0 == tmpInternal->parent)  // we reach the root node and need a new root
            {
                newRootOffset = CreateNewRoot_int(tree, &root, tmpInternal, tmpInternalOffset);
                tmpInternal->parent = newRootOffset;
                height++;
            }
            // full, split it
            newInternalOffset = CreateNewInternal_int(tree, tmpInternal, tmpInternalOffset, &newInternal);
            CopyInternal_int(&newIndex, tmpInternal, &newInternal);
            if (0 == KeyCmp_int(tmpInternal->children[0].key, newIndex.key))
            {
                ResetIndexR_int(tree, tmpInternal, newIndex.key, tmpInternalOffset);
            }
            WriteBlock(tree->path, tmpInternal, tmpInternalOffset, sizeof(internal_t_int));
            WriteBlock(tree->path, &newInternal, newInternalOffset, sizeof(internal_t_int));
            // after CopyInternal_int, we have to reset children's parent
            ResetIndexParent_int(tree, &newInternal, newInternalOffset);  // reset the children's parent as the newInternal node
            // move to the upper level
            // update newIndex
            newIndex.key = newInternal.children[0].key;
            newIndex.child = newInternalOffset;
            tmpInternalOffset = tmpInternal->parent;
#ifdef NOBUFFER
            free(tmpInternal);
#endif
            tmpInternal = (internal_t_int *)ReadBlock(tree->path, tmpInternalOffset, sizeof(internal_t_int));
            height--;
        }
#ifdef NOBUFFER
        free(tmpInternal);
#endif
    }  // case 2: split tree
    WriteBlock(tree->path, &tree->meta, META_OFFSET, sizeof(meta_t));  // update meta data
    return 0;
}

off_t SearchIndex_int(BPlusTree tree, my_key_t_int key)
{
    off_t offset = tree->meta.rootOffset;
    int i, height = tree->meta.height;
    internal_t_int *node;
    //index_t_int *index;
    while (height > 1)
    {
        node = (internal_t_int *)ReadBlock(tree->path, offset, sizeof(internal_t_int));
        for (i = (int)node->n - 1; i >= 0; i--)  // ignore the value of node->children[0]
        {
            //index = &node->children[i];
            offset = node->children[i].child;
            if (KeyCmp_int(node->children[i].key, key) <= 0)
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

off_t SearchLeaf_int(BPlusTree tree, off_t parent, my_key_t_int key)
{
    off_t offset;
    internal_t_int *node;
    int i;
    node = (internal_t_int *)ReadBlock(tree->path, parent, sizeof(internal_t_int));
    for (i = (int)node->n - 1; i >= 0; i--)
    {
        offset = node->children[i].child;
        if (1 == tree->meta.leafNum || KeyCmp_int(node->children[i].key, key) <= 0)
        {
            break;
        }
    }
#ifdef NOBUFFER
    free(node);
#endif
    return offset;
}

int KeyCmp_int(my_key_t_int A, my_key_t_int B)
{
    return KeyValueCmp(A.key, B.key);
}

void InsertIntoLeaf_int(leaf_t_int *leaf, record_t_int *newRecord)
{
    int i;
    i = leaf->n - 1;
    while (i >= 0 && KeyCmp_int(leaf->children[i].key, newRecord->key) > 0)
    {
        leaf->children[i + 1] = leaf->children[i];
        i--;
    }
    leaf->children[i + 1] = *newRecord;
    leaf->n++;
}

void InsertIntoInternal_int(internal_t_int *internal, index_t_int index)
{
    int i;
    i = internal->n - 1;
    while (i >= 0 && KeyCmp_int(internal->children[i].key, index.key) > 0)
    {
        internal->children[i + 1] = internal->children[i];
        i--;
    }
    internal->children[i + 1] = index;
    internal->n++;
}


value_t Search_int(BPlusTree tree, my_key_t_int key)
{
    off_t parent, offset;
    leaf_t_int *leaf;
    int i, compareRes;
    parent = SearchIndex_int(tree, key);
    offset = SearchLeaf_int(tree, parent, key);
    leaf = (leaf_t_int *)ReadBlock(tree->path, offset, sizeof(leaf_t_int));
    for (i = 0; i < (int)leaf->n; i++)
    {
        compareRes = KeyCmp_int(leaf->children[i].key, key);
        if (compareRes == 0)
        {
            return leaf->children[i].value;
        }
    }
#ifdef NOBUFFER
    free(leaf);
#endif
    return 0;
}

int SearchKeyInLeaf_int(my_key_t_int key, leaf_t_int *leaf)
{
    int i, compareRes;
    for (i = 0; i < (int)leaf->n; i++)
    {
        compareRes = KeyCmp_int(leaf->children[i].key, key);
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

void CopyLeaf_int(leaf_t_int *leaf, leaf_t_int *newLeaf, record_t_int tmpRecord)  // copy the right half of a full leaf to a new leaf node
{
    int i, j;
    record_t_int swapRecord;
    if (KeyCmp_int(leaf->children[leaf->n - 1].key, tmpRecord.key) > 0)
    {
        // swap
        swapRecord = leaf->children[leaf->n - 1];
        leaf->n--;
        InsertIntoLeaf_int(leaf, &tmpRecord);
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


void ResetIndexR_int(BPlusTree tree, internal_t_int *tmpInternal, my_key_t_int key, off_t offset)
{
    int i;
    index_t_int newIndex;
    off_t parent;
    internal_t_int *node;
    newIndex.key = key;
    newIndex.child = offset;
    parent = tmpInternal->parent;
    while (parent)
    {
        node = (internal_t_int *)ReadBlock(tree->path, parent, sizeof(internal_t_int));
        for (i = 0; i < (int)node->n; i++)
        {
            if (node->children[i].child == newIndex.child)
            {
                node->children[i] = newIndex;
                break;
            }
        }
        WriteBlock(tree->path, node, parent, sizeof(internal_t_int));
#ifdef NOBUFFER
        free(node);
#endif
        parent = node->parent;
    }
}

void CopyInternal_int(index_t_int *newIndex, internal_t_int *tmpInternal, internal_t_int *newInternal)
{
    int i, j;
    index_t_int swapIndex;
    if (KeyCmp_int(newIndex->key, tmpInternal->children[tmpInternal->n - 1].key) > 0)
    {
        swapIndex = *newIndex;
    }
    else
    {
        swapIndex = tmpInternal->children[tmpInternal->n - 1];
        tmpInternal->n--;
        InsertIntoInternal_int(tmpInternal, *newIndex);
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

off_t CreateNewLeaf_int(BPlusTree tree, leaf_t_int *leaf, off_t offset, leaf_t_int *newLeaf)
{
    off_t newLeafOffset;
    leaf_t_int *newLeafNext;
    newLeafOffset = AllocLeaf_int(tree, newLeaf);
    newLeaf->parent = leaf->parent;
    newLeaf->next = leaf->next;
    leaf->next = newLeafOffset;
    newLeaf->prev = offset;
    if (newLeaf->next)
    {
        newLeafNext = (leaf_t_int *)ReadBlock(tree->path, newLeaf->next, sizeof(leaf_t_int));
        newLeafNext->prev = newLeafOffset;
        WriteBlock(tree->path, newLeafNext, newLeaf->next, sizeof(leaf_t_int));
#ifdef NOBUFFER
        free(newLeafNext);
#endif
    }
    return newLeafOffset;
}

off_t CreateNewInternal_int(BPlusTree tree, internal_t_int *internal, off_t offset, internal_t_int *newInternal)
{
    off_t newInternalOffset;
    internal_t_int *newInternalNext;
    newInternalOffset = AllocInternal_int(tree, newInternal);
    newInternal->parent = internal->parent;
    newInternal->next = internal->next;
    internal->next = newInternalOffset;
    newInternal->prev = offset;
    if (newInternal->next)
    {
        newInternalNext = (internal_t_int *)ReadBlock(tree->path, newInternal->next, sizeof(internal_t_int));
        newInternalNext->prev = newInternalOffset;
        WriteBlock(tree->path, newInternalNext, newInternal->next, sizeof(internal_t_int));
#ifdef NOBUFFER
        free(newInternalNext);
#endif
    }
    return newInternalOffset;
}

off_t CreateNewRoot_int(BPlusTree tree, internal_t_int *root, internal_t_int *tmpInternal, off_t tmpInternalOffset)
{
    off_t newRootOffset;
    newRootOffset = AllocInternal_int(tree, root);
    tree->meta.rootOffset = newRootOffset;
    tree->meta.height++;
    root->prev = root->next = root->parent = 0;
    // update children of root
    root->children[0].key = tmpInternal->children[0].key;
    root->children[0].child = tmpInternalOffset;
    // update root->n
    root->n = 1;
    WriteBlock(tree->path, root, newRootOffset, sizeof(internal_t_int));
    return newRootOffset;
}

void ResetIndexParent_int(BPlusTree tree, internal_t_int *internal, off_t offset)  // reset the children's parent as the newInternal node
{
    int i;
    internal_t_int *child;
    off_t childOffset;
    for (i = 0; i < (int)internal->n; i++)
    {
        childOffset = internal->children[i].child;
        child = (internal_t_int *)ReadBlock(tree->path, childOffset, sizeof(internal_t_int));
        child->parent = offset;
        WriteBlock(tree->path, child, childOffset, BLOCK_SIZE);
#ifdef NOBUFFER
        free(child);
#endif
    }
}

int Remove_int(BPlusTree tree, my_key_t_int key)
{
    internal_t_int *parent;
    leaf_t_int *leaf, *sibling;
    off_t offset, parentOffset;
    size_t minChildrenNum;
    my_key_t_int oldKey;
    int i, compareRes, keyPos, done = 0;
    // First search the key we are going to remove
    parentOffset = SearchIndex_int(tree, key);
    offset = SearchLeaf_int(tree, parentOffset, key);
    leaf = (leaf_t_int *)ReadBlock(tree->path, offset, sizeof(leaf_t_int));
    for (i = 0; i < (int)leaf->n; i++)
    {
        compareRes = KeyCmp_int(leaf->children[i].key, key);
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

    minChildrenNum = tree->meta.leafNum == 1 ? 0 : (tree->meta.order % 2 ? tree->meta.order / 2 + 1 : tree->meta.order / 2);
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
            done = BorrowKey_int(tree, 0, leaf);
        }
        if (!done && leaf->next)  // try to borrow from right
        {
            done = BorrowKey_int(tree, 1, leaf);
        }
        // try to merge
        if (!done)
        {
            parent = (internal_t_int *)ReadBlock(tree->path, parentOffset, sizeof(internal_t_int));
            if (parent->children[parent->n - 1].child == offset)  // leaf is the last child of parent, merge it with leaf->prev
            {
                sibling = (leaf_t_int *)ReadBlock(tree->path, leaf->prev, sizeof(leaf_t_int));
                MergeLeaves_int(sibling, leaf);
                RemoveLeaf_int(tree, sibling, leaf);
                WriteBlock(tree->path, sibling, leaf->prev, sizeof(leaf_t_int));
                // remove index from leaf's parent
                oldKey = leaf->children[0].key;
                RemoveIndex_int(tree, parent, parentOffset, oldKey);
            }
            else  // merge leaf with leaf->next
            {
                sibling = (leaf_t_int *)ReadBlock(tree->path, leaf->next, sizeof(leaf_t_int));
                MergeLeaves_int(leaf, sibling);
                RemoveLeaf_int(tree, leaf, sibling);
                WriteBlock(tree->path, leaf, offset, sizeof(leaf_t_int));
                oldKey = sibling->children[0].key;
#ifdef NOBUFFER
                free(parent);
#endif
                // remove index from sibling's parent
                parentOffset = sibling->parent;
                parent = (internal_t_int *)ReadBlock(tree->path, parentOffset, sizeof(internal_t_int));
                RemoveIndex_int(tree, parent, parentOffset, oldKey);
            }
#ifdef NOBUFFER
            free(sibling);
#endif
        }
        else
        {  // succeed borrowing, write back leaf
            WriteBlock(tree->path, leaf, offset, sizeof(leaf_t_int));
        }
    }
    else  // no need to borrow or merge
    {
        WriteBlock(tree->path, leaf, offset, sizeof(leaf_t_int));
    }
#ifdef NOBUFFER
    free(leaf);
#endif
    return 0;
}

int BorrowKey_int(BPlusTree tree, int borrowFromRight, leaf_t_int *leaf)
{
    off_t siblingOffset;
    leaf_t_int *sibling;
    int i, borrowPos, storePos;
    size_t minChildrenNum;
    minChildrenNum = tree->meta.order % 2 ? tree->meta.order / 2 + 1 : tree->meta.order / 2;
    siblingOffset = borrowFromRight ? leaf->next : leaf->prev;
    sibling = (leaf_t_int *)ReadBlock(tree->path, siblingOffset, sizeof(leaf_t_int));
    if (sibling->n <= minChildrenNum)  // cannot borrow
    {
#ifdef NOBUFFER
        free(sibling);
#endif
        return 0;
    }
    if (borrowFromRight)
    {
        borrowPos = 0;
        storePos = leaf->n;
        UpdateIndexChild_int(tree, sibling->parent, sibling->children[0].key, sibling->children[1].key);
    }
    else
    {
        borrowPos = sibling->n - 1;
        storePos = 0;
        UpdateIndexChild_int(tree, leaf->parent, leaf->children[0].key, sibling->children[borrowPos].key);
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
    WriteBlock(tree->path, sibling, siblingOffset, sizeof(leaf_t_int));
#ifdef NOBUFFER
    free(sibling);
#endif
    return 1;  // succeeded
}

void UpdateIndexChild_int(BPlusTree tree, off_t parentOffset, my_key_t_int oldKey, my_key_t_int newKey)
{
    internal_t_int *parent;
    int i;
    parent = (internal_t_int *)ReadBlock(tree->path, parentOffset, sizeof(internal_t_int));
    for (i = (int)parent->n - 1; i >= 0; i--)
    {
        if (KeyCmp_int(parent->children[i].key, oldKey) <= 0 || 0 == i)  // found old key
        {
            parent->children[i].key = newKey;
            break;
        }
    }
    WriteBlock(tree->path, parent, parentOffset, sizeof(internal_t_int));
    if (0 == i && 0 != parent->parent)  // recursively update child
    {
        UpdateIndexChild_int(tree, parent->parent, oldKey, newKey);
    }
#ifdef NOBUFFER
    free(parent);
#endif
}

int MergeLeaves_int(leaf_t_int *left, leaf_t_int *right)
{
    int i, j;
    for (i = 0, j = (int)left->n; i < (int)right->n; i++, j++)
    {
        left->children[j] = right->children[i];
    }
    left->n += right->n;
    return 0;
}

int RemoveLeaf_int(BPlusTree tree, leaf_t_int *left, leaf_t_int *right)
{
    leaf_t_int *rightNext;
    left->next = right->next;
    UnallocLeaf(tree);
    if (right->next)
    {
        rightNext = (leaf_t_int *)ReadBlock(tree->path, right->next, sizeof(leaf_t_int));
        rightNext->prev = right->prev;
        WriteBlock(tree->path, rightNext, right->next, sizeof(leaf_t_int));
    }
    WriteBlock(tree->path, &tree->meta, META_OFFSET, BLOCK_SIZE);
    return 0;
}

int RemoveIndex_int(BPlusTree tree, internal_t_int *node, off_t offset, my_key_t_int oldKey)
{
    size_t minChildrenNum;
    internal_t_int *parent, *sibling;
    my_key_t_int keytoRemove;
    off_t parentOffset;
    int i, keyPos, compareRes, done = 0;
    for(i = (int)node->n - 1; i >= 0; i--)
    {
        compareRes = KeyCmp_int(node->children[i].key, oldKey);
        if (compareRes <= 0 || 0 == i)  // @NOTE seems dangerours
        {
            keyPos = i;
            break;
        }
    }
    minChildrenNum = node->parent == 0 ? 1 : (tree->meta.order % 2 ? tree->meta.order / 2 + 1 : tree->meta.order / 2);
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
        // a new root
        node = (internal_t_int *)ReadBlock(tree->path, tree->meta.rootOffset, sizeof(internal_t_int));
        node->parent = node->prev = node->next = 0;
        WriteBlock(tree->path, node, tree->meta.rootOffset, sizeof(internal_t_int));
#ifdef NOBUFFER
        free(node);
#endif
        return 0;
    }

    if (node->n < minChildrenNum)  // Oops, we have to merge the node with a neighbor or borrow a child from its sibling
    {
        if (node->prev)  // try to borrow from left
        {
            done = BorrowKeyFromInternal_int(tree, 0, node, offset);
        }
        if (!done && node->next)  // try to borrow from right
        {
            done = BorrowKeyFromInternal_int(tree, 1, node, offset);
        }
        // try to merge
        if (!done)
        {
            parent = (internal_t_int *)ReadBlock(tree->path, node->parent, sizeof(internal_t_int));
            if (parent->children[parent->n - 1].child == offset)  // ndoe is the last child of parent, merge it with node->prev
            {
                if (0 == node->prev)
                {
                    fprintf(stderr, "Fatal Error: bug in the tree structure.\n");
                    exit(-1);
                }
                sibling = (internal_t_int *)ReadBlock(tree->path, node->prev, sizeof(internal_t_int));
                MergeInternals_int(sibling, node);
                RemoveInternal_int(tree, sibling, node);
                WriteBlock(tree->path, sibling, node->prev, sizeof(internal_t_int));
                keytoRemove = node->children[0].key;
                RemoveIndex_int(tree, parent, node->parent, keytoRemove);
            }
            else  // merge node with node->next
            {
                if (0 == node->next)
                {
                    fprintf(stderr, "Fatal Error: bug in the tree structure.\n");
                    exit(-1);
                }
                sibling = (internal_t_int *)ReadBlock(tree->path, node->next, sizeof(internal_t_int));
                MergeInternals_int(node, sibling);
                RemoveInternal_int(tree, node, sibling);
                WriteBlock(tree->path, node, offset, sizeof(internal_t_int));
                keytoRemove = sibling->children[0].key;
#ifdef NOBUFFER
                free(parent);
#endif
                parentOffset = sibling->parent;
                parent = (internal_t_int *)ReadBlock(tree->path, node->parent, sizeof(internal_t_int));
                RemoveIndex_int(tree, parent, parentOffset, keytoRemove);
            }
#ifdef NOBUFFER
            free(sibling);
#endif
        }
        else  // succeed borrowing, write back leaf
        {
            WriteBlock(tree->path, node, offset, sizeof(internal_t_int));
        }
    }
    else   // no need to borrow or merge
    {
        WriteBlock(tree->path, node, offset, sizeof(internal_t_int));
    }
#ifdef NOBUFFER
    free(node);
#endif
    return 0;
}

int BorrowKeyFromInternal_int(BPlusTree tree, int borrowFromRight, internal_t_int *node, off_t offset)
{
    off_t siblingOffset;
    internal_t_int *sibling;
    int i, borrowPos, storePos;
    size_t minChildrenNum;

    minChildrenNum = tree->meta.order % 2 ? tree->meta.order / 2 + 1 : tree->meta.order / 2;
    siblingOffset = borrowFromRight ? node->next : node->prev;
    sibling = (internal_t_int *)ReadBlock(tree->path, siblingOffset, sizeof(internal_t_int));
    if (sibling->n <= minChildrenNum)  // cannot borrow
    {
        return 0;
    }
    if (borrowFromRight)
    {
        borrowPos = 0;
        storePos = node->n;
        UpdateIndexChild_int(tree, sibling->parent, sibling->children[0].key, sibling->children[1].key);
    }
    else
    {
        borrowPos = sibling->n - 1;
        storePos = 0;
        UpdateIndexChild_int(tree, node->parent, node->children[0].key, sibling->children[borrowPos].key);
    }
    // insert into node
    for (i = (int)node->n - 1; i >= storePos; i--)  // shift leaf->children[i] right
    {
        node->children[i + 1] = node->children[i];
    }
    node->children[storePos] = sibling->children[borrowPos];
    node->n++;
    // update the child's parent
    ResetIndexParent_int(tree, node, offset);
    // remove from sibling
    for (i = borrowPos + 1; i < (int)sibling->n; i++)
    {
        sibling->children[i - 1] = sibling->children[i];
    }
    sibling->n--;
    WriteBlock(tree->path, sibling, siblingOffset, sizeof(internal_t_int));
#ifdef NOBUFFER
    free(sibling);
#endif
    return 1;
}

void MergeInternals_int(internal_t_int *left, internal_t_int *right)
{
    int i, j;
    for (i = 0, j = (int)left->n; i < (int)right->n; i++, j++)
    {
        left->children[j] = right->children[i];
    }
    left->n += right->n;
}

void RemoveInternal_int(BPlusTree tree, internal_t_int *left, internal_t_int *right)
{
    internal_t_int *rightNext;
    off_t leftOffset = right->prev;
    left->next = right->next;
    UnallocInternal(tree);
    if (right->next)
    {
        rightNext = (internal_t_int *)ReadBlock(tree->path, right->next, sizeof(internal_t_int));
        rightNext->prev = right->prev;
        WriteBlock(tree->path, rightNext, right->next, sizeof(internal_t_int));
    }
    //@TODO reset chilren's parent after merge
    ResetIndexParent_int(tree, left, leftOffset);
    WriteBlock(tree->path, &tree->meta, META_OFFSET, BLOCK_SIZE);
}
