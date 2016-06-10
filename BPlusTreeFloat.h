#ifndef BPLUSTREEFLOAT_H
#define BPLUSTREEFLOAT_H
#define NOBUFFER
#include "MiniSQL.h"
#include "BPlusTree.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//typedef _off_t off_t
// the following definition of offsets might be replaced in the real work
#define TREE_ORDER_float  ((BLOCK_SIZE - 3 * sizeof(off_t) - sizeof(size_t)) / sizeof(record_t_float))
#define KeyValueCmp(a, b) _Generic(a, int: IntKeyCmp, float: FloatKeyCmp, char *: StringKeyCmp)(a, b)
// key and value definition
typedef off_t value_t;  // value type, default int

// tree structure
typedef struct index_t_float index_t_float;
struct index_t_float
{
    my_key_t_float key;
    off_t child;
};

typedef struct record_t_float record_t_float;
struct record_t_float
{
    my_key_t_float key;
    value_t value;
};

typedef struct leaf_t_float leaf_t_float;
struct leaf_t_float
{
    off_t parent;
    off_t next;
    off_t prev;
    size_t n;
    record_t_float children[TREE_ORDER_float];
};

typedef struct internal_t_float internal_t_float;
struct internal_t_float
{
    off_t parent;
    off_t next;
    off_t prev;
    size_t n;
    index_t_float children[TREE_ORDER_float];
};

// ============= other modules can invoke the following functions ==============
void InitTree_float(BPlusTree tree, char *path, enum DataType type);
int Insert_float(BPlusTree tree, my_key_t_float key, value_t value);
value_t Search_float(BPlusTree tree, my_key_t_float key);
int Remove_float(BPlusTree tree, my_key_t_float key);
// =============================================================================
// initialize tree
off_t AllocLeaf_float(BPlusTree tree, leaf_t_float *node);
off_t AllocInternal_float(BPlusTree tree, internal_t_float *node);
// Insert_float
off_t SearchIndex_float(BPlusTree tree, my_key_t_float key);
off_t SearchLeaf_float(BPlusTree tree, off_t parent, my_key_t_float key);
int SearchKeyInLeaf_float(my_key_t_float key, leaf_t_float *leaf);
void CopyLeaf_float(leaf_t_float *leaf, leaf_t_float *newLeaf, record_t_float tmpRecord);  // copy the right half of a full leaf to a new leaf node
void CopyInternal_float(index_t_float *newIndex, internal_t_float *tmpInternal, internal_t_float *newInternal);  // copy the rignt half of a full internal to a new one
off_t CreateNewLeaf_float(BPlusTree tree, leaf_t_float *leaf, off_t offset, leaf_t_float *newLeaf);
off_t CreateNewInternal_float(BPlusTree tree, internal_t_float *internal, off_t offset, internal_t_float *newInternal);
off_t CreateNewRoot_float(BPlusTree tree, internal_t_float *root, internal_t_float *tmpInternal, off_t tmpInternalOffset);
void ResetIndexR_float(BPlusTree tree, internal_t_float *tmpInternal, my_key_t_float key, off_t offset);
void ResetIndexParent_float(BPlusTree tree, internal_t_float *newInternal, off_t newInternalOffset);  // reset the children's parent as the newInternal node
int KeyCmp_float(my_key_t_float A, my_key_t_float B);
void InsertIntoLeaf_float(leaf_t_float *leaf, record_t_float *newRecord);
void InsertIntoInternal_float(internal_t_float *internal, index_t_float index);
// Remove_float
int BorrowKey_float(BPlusTree tree, int borrowFromRight, leaf_t_float *leaf);
void UpdateIndexChild_float(BPlusTree tree, off_t parentOffset, my_key_t_float oldKey, my_key_t_float newKey);
int MergeLeaves_float(leaf_t_float *left, leaf_t_float *right);
int RemoveLeaf_float(BPlusTree tree, leaf_t_float *left, leaf_t_float *right);
int RemoveIndex_float(BPlusTree tree, internal_t_float *node, off_t offset, my_key_t_float oldKey);
int BorrowKeyFromInternal_float(BPlusTree tree, int borrowFromRight, internal_t_float *node, off_t offset);
void MergeInternals_float(internal_t_float *left, internal_t_float *right);
void RemoveInternal_float(BPlusTree tree, internal_t_float *left, internal_t_float *right);
#endif
