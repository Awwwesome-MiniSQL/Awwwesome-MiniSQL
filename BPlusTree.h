#ifndef BPLUSTREE_H
#define BPLUSTREE_H
#define NOBUFFER
#include "MiniSQL.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//typedef _off_t off_t
// the following definition of offsets might be replaced in the real work
#define TREE_ORDER  (BLOCK_SIZE - 3 * sizeof(off_t) - sizeof(size_t)) / sizeof(record_t)
#define META_OFFSET 0  // this means one file contains exactly one tree, and the beginning of a file is the meta data
#define BLOCK_OFFSET META_OFFSET + BLOCK_SIZE
#define SIZE_NO_CHILDREN sizeof(leaf_t) + TREE_ORDER * sizeof(record_t)
#define KeyValueCmp(a, b) _Generic(a, int: IntKeyCmp, float: FloatKeyCmp, char *: StringKeyCmp)(a, b)
// =====================ReadBlock and WriteBlock are not my work ===============
// @NOTE here we need to invoke Buffer module to read / write blocks
void *ReadBlock(char *fileName, off_t offset, size_t size);  // return a pointer which points to a block in memory
int WriteBlock(char *fileName, void *block, off_t offset, size_t size);  // return 1 if succeeded or 0 if not
// =============================================================================
// key and value definition
typedef off_t value_t;  // value type, default int
typedef struct my_key_t my_key_t;  // key type (int, float, varchar)
struct my_key_t
{
    int key;
    //float key;
    //char key[256];
    //size_t size;
};

// tree structure
typedef struct index_t index_t;
struct index_t
{
    my_key_t key;
    off_t child;
};

typedef struct record_t record_t;
struct record_t
{
    my_key_t key;
    value_t value;
};

typedef struct leaf_t leaf_t;
struct leaf_t
{
    off_t parent;
    off_t next;
    off_t prev;
    size_t n;
    record_t children[TREE_ORDER];
};

typedef struct internal_t internal_t;
struct internal_t
{
    off_t parent;
    off_t next;
    off_t prev;
    size_t n;
    index_t children[TREE_ORDER];
};

typedef struct meta_t meta_t;
struct meta_t
{
    size_t order;
    size_t valueSize;
    size_t keySize;
    size_t internalNum;
    size_t leafNum;
    size_t height;
    off_t slot;
    off_t rootOffset;
    off_t leafOffset;
    enum DataType type;
};

typedef struct tree_t *BPlusTree;
struct tree_t
{
    //FILE *fp;  // multi-level file handling
    //int fpLevel; // the level of current file, to avoid open for many times
    char path[1024];  // path to the index file
    meta_t meta;  // meta data
};

// ============= other modules can invoke the following functions ==============
void InitTree(BPlusTree tree, char *path, enum DataType type);
int Insert(BPlusTree tree, my_key_t key, value_t value);
value_t Search(BPlusTree tree, my_key_t key);
int Remove(BPlusTree tree, my_key_t key);
// =============================================================================
// initialize tree
off_t AllocLeaf(BPlusTree tree, leaf_t *node);
off_t AllocInternal(BPlusTree tree, internal_t *node);
off_t AllocSize(BPlusTree tree, size_t size);
// Insert
off_t SearchIndex(BPlusTree tree, my_key_t key);
off_t SearchLeaf(BPlusTree tree, off_t parent, my_key_t key);
int SearchKeyInLeaf(my_key_t key, leaf_t *leaf);
void CopyLeaf(leaf_t *leaf, leaf_t *newLeaf, record_t tmpRecord);  // copy the right half of a full leaf to a new leaf node
void CopyInternal(index_t *newIndex, internal_t *tmpInternal, internal_t *newInternal);  // copy the rignt half of a full internal to a new one
off_t CreateNewLeaf(BPlusTree tree, leaf_t *leaf, off_t offset, leaf_t *newLeaf);
off_t CreateNewInternal(BPlusTree tree, internal_t *internal, off_t offset, internal_t *newInternal);
off_t CreateNewRoot(BPlusTree tree, internal_t *root, internal_t *tmpInternal, off_t tmpInternalOffset);
void ResetIndexR(BPlusTree tree, internal_t *tmpInternal, my_key_t key, off_t offset);
void ResetIndexParent(BPlusTree tree, internal_t *newInternal, off_t newInternalOffset);  // reset the children's parent as the newInternal node
int KeyCmp(my_key_t A, my_key_t B);
int IntKeyCmp(int A, int B);
int FloatKeyCmp(float A, float B);
int StringKeyCmp(char *A, char *B);
void InsertIntoLeaf(leaf_t *leaf, record_t *newRecord);
void InsertIntoInternal(internal_t *internal, index_t index);
// Remove
int BorrowKey(BPlusTree tree, int borrowFromRight, leaf_t *leaf);
void UpdateIndexChild(BPlusTree tree, off_t parentOffset, my_key_t oldKey, my_key_t newKey);
int MergeLeaves(leaf_t *left, leaf_t *right);
int RemoveLeaf(BPlusTree tree, leaf_t *left, leaf_t *right);
int RemoveIndex(BPlusTree tree, internal_t *node, off_t offset, my_key_t oldKey);
void UnallocLeaf(BPlusTree tree);
void UnallocInternal(BPlusTree tree);
int BorrowKeyFromInternal(BPlusTree tree, int borrowFromRight, internal_t *node, off_t offset);
void MergeInternals(internal_t *left, internal_t *right);
void RemoveInternal(BPlusTree tree, internal_t *left, internal_t *right);
#endif
