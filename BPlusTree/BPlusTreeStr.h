#ifndef BPLUSTREESTR_H
#define BPLUSTREESTR_H
#define NOBUFFER
#include "../MiniSQL.h"
#include "BPlusTree.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//typedef _off_t off_t
// the following definition of offsets might be replaced in the real work
#define TREE_ORDER_str  ((BLOCK_SIZE - 3 * sizeof(off_t) - sizeof(size_t)) / sizeof(record_t_str))
#define KeyValueCmp(a, b) _Generic(a, int: IntKeyCmp, float: FloatKeyCmp, char *: StringKeyCmp)(a, b)
// key and value definition
typedef off_t value_t;  // value type, default int

// tree structure
typedef struct index_t_str index_t_str;
struct index_t_str
{
    my_key_t_str key;
    off_t child;
};

typedef struct record_t_str record_t_str;
struct record_t_str
{
    my_key_t_str key;
    value_t value;
};

typedef struct leaf_t_str leaf_t_str;
struct leaf_t_str
{
    off_t parent;
    off_t next;
    off_t prev;
    size_t n;
    record_t_str children[TREE_ORDER_str];
};

typedef struct internal_t_str internal_t_str;
struct internal_t_str
{
    off_t parent;
    off_t next;
    off_t prev;
    size_t n;
    index_t_str children[TREE_ORDER_str];
};

// ============= other modules can invoke the following functions ==============
void InitTree_str(BPlusTree tree, char *path, enum DataType type);
int Insert_str(BPlusTree tree, my_key_t_str key, value_t value);
value_t Search_str(BPlusTree tree, my_key_t_str key);
int Remove_str(BPlusTree tree, my_key_t_str key);
// =============================================================================
// initialize tree
off_t AllocLeaf_str(BPlusTree tree, leaf_t_str *node);
off_t AllocInternal_str(BPlusTree tree, internal_t_str *node);
// Insert_str
off_t SearchIndex_str(BPlusTree tree, my_key_t_str key);
off_t SearchLeaf_str(BPlusTree tree, off_t parent, my_key_t_str key);
int SearchKeyInLeaf_str(my_key_t_str key, leaf_t_str *leaf);
void CopyLeaf_str(leaf_t_str *leaf, leaf_t_str *newLeaf, record_t_str tmpRecord);  // copy the right half of a full leaf to a new leaf node
void CopyInternal_str(index_t_str *newIndex, internal_t_str *tmpInternal, internal_t_str *newInternal);  // copy the rignt half of a full internal to a new one
off_t CreateNewLeaf_str(BPlusTree tree, leaf_t_str *leaf, off_t offset, leaf_t_str *newLeaf);
off_t CreateNewInternal_str(BPlusTree tree, internal_t_str *internal, off_t offset, internal_t_str *newInternal);
off_t CreateNewRoot_str(BPlusTree tree, internal_t_str *root, internal_t_str *tmpInternal, off_t tmpInternalOffset);
void ResetIndexR_str(BPlusTree tree, internal_t_str *tmpInternal, my_key_t_str key, off_t offset);
void ResetIndexParent_str(BPlusTree tree, internal_t_str *newInternal, off_t newInternalOffset);  // reset the children's parent as the newInternal node
int KeyCmp_str(my_key_t_str A, my_key_t_str B);
void InsertIntoLeaf_str(leaf_t_str *leaf, record_t_str *newRecord);
void InsertIntoInternal_str(internal_t_str *internal, index_t_str index);
// Remove_str
int BorrowKey_str(BPlusTree tree, int borrowFromRight, leaf_t_str *leaf);
void UpdateIndexChild_str(BPlusTree tree, off_t parentOffset, my_key_t_str oldKey, my_key_t_str newKey);
int MergeLeaves_str(leaf_t_str *left, leaf_t_str *right);
int RemoveLeaf_str(BPlusTree tree, leaf_t_str *left, leaf_t_str *right);
int RemoveIndex_str(BPlusTree tree, internal_t_str *node, off_t offset, my_key_t_str oldKey);
int BorrowKeyFromInternal_str(BPlusTree tree, int borrowFromRight, internal_t_str *node, off_t offset);
void MergeInternals_str(internal_t_str *left, internal_t_str *right);
void RemoveInternal_str(BPlusTree tree, internal_t_str *left, internal_t_str *right);
#endif
