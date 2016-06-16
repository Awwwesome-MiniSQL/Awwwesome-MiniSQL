#ifndef BPLUSTREEINT_H
#define BPLUSTREEINT_H
#define NOBUFFER
#include "../MiniSQL.h"
#include "BPlusTree.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//typedef _off_t off_t
// the following definition of offsets might be replaced in the real work
#define KeyValueCmp(a, b) _Generic(a, int: IntKeyCmp, float: FloatKeyCmp, char *: StringKeyCmp)(a, b)
// key and value definition
typedef off_t value_t;  // value type, default int

// tree structure
typedef struct index_t_int index_t_int;
struct index_t_int
{
    my_key_t_int key;
    off_t child;
};

typedef struct internal_t_int internal_t_int;
struct internal_t_int
{
    off_t parent;
    off_t next;
    off_t prev;
    size_t n;
    index_t_int children[TREE_ORDER_int];
};

// ============= other modules can invoke the following functions ==============
//void InitTree_int(BPlusTree tree, char *path, enum DataType type);
int Insert_int(BPlusTree tree, my_key_t_int key, value_t value);
value_t Search_int(BPlusTree tree, my_key_t_int key);
int Remove_int(BPlusTree tree, my_key_t_int key);
// =============================================================================
// initialize tree
off_t AllocLeaf_int(BPlusTree tree, leaf_t_int *node);
off_t AllocInternal_int(BPlusTree tree, internal_t_int *node);
// Insert_int
off_t SearchIndex_int(BPlusTree tree, my_key_t_int key);
off_t SearchLeaf_int(BPlusTree tree, off_t parent, my_key_t_int key);
int SearchKeyInLeaf_int(my_key_t_int key, leaf_t_int *leaf);
void CopyLeaf_int(leaf_t_int *leaf, leaf_t_int *newLeaf, record_t_int tmpRecord);  // copy the right half of a full leaf to a new leaf node
void CopyInternal_int(index_t_int *newIndex, internal_t_int *tmpInternal, internal_t_int *newInternal);  // copy the rignt half of a full internal to a new one
off_t CreateNewLeaf_int(BPlusTree tree, leaf_t_int *leaf, off_t offset, leaf_t_int *newLeaf);
off_t CreateNewInternal_int(BPlusTree tree, internal_t_int *internal, off_t offset, internal_t_int *newInternal);
off_t CreateNewRoot_int(BPlusTree tree, internal_t_int *root, internal_t_int *tmpInternal, off_t tmpInternalOffset);
void ResetIndexR_int(BPlusTree tree, internal_t_int *tmpInternal, my_key_t_int key, off_t offset);
void ResetIndexParent_int(BPlusTree tree, internal_t_int *newInternal, off_t newInternalOffset);  // reset the children's parent as the newInternal node
int KeyCmp_int(my_key_t_int A, my_key_t_int B);
void InsertIntoLeaf_int(leaf_t_int *leaf, record_t_int *newRecord);
void InsertIntoInternal_int(internal_t_int *internal, index_t_int index);
// Remove_int
int BorrowKey_int(BPlusTree tree, int borrowFromRight, leaf_t_int *leaf);
void UpdateIndexChild_int(BPlusTree tree, off_t parentOffset, my_key_t_int oldKey, my_key_t_int newKey);
int MergeLeaves_int(leaf_t_int *left, leaf_t_int *right);
int RemoveLeaf_int(BPlusTree tree, leaf_t_int *left, leaf_t_int *right);
int RemoveIndex_int(BPlusTree tree, internal_t_int *node, off_t offset, my_key_t_int oldKey);
int BorrowKeyFromInternal_int(BPlusTree tree, int borrowFromRight, internal_t_int *node, off_t offset);
void MergeInternals_int(internal_t_int *left, internal_t_int *right);
void RemoveInternal_int(BPlusTree tree, internal_t_int *left, internal_t_int *right);
#endif
