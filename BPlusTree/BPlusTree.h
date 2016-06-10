#ifndef BPLUSTREE_H
#define BPLUSTREE_H
#define NOBUFFER
#include "../MiniSQL.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//typedef _off_t off_t
// the following definition of offsets might be replaced in the real work
#define META_OFFSET 0  // this means one file contains exactly one tree, and the beginning of a file is the meta data
#define BLOCK_OFFSET META_OFFSET + BLOCK_SIZE
#define InsertIndex(a, b, c) _Generic(b, my_key_t_int: Insert_int, my_key_t_float: Insert_float, my_key_t_str: Insert_str)(a, b, c)
#define SearchIndex(a, b) _Generic(b, my_key_t_int: Search_int, my_key_t_float: Search_float, my_key_t_str: Search_str)(a, b)
#define RemoveIndex(a, b) _Generic(b, my_key_t_int: Remove_int, my_key_t_float: Remove_float, my_key_t_str: Remove_str)(a, b)

#define TREE_ORDER_float  ((BLOCK_SIZE - 3 * sizeof(off_t) - sizeof(size_t)) / sizeof(record_t_float))
#define TREE_ORDER_int  ((BLOCK_SIZE - 3 * sizeof(off_t) - sizeof(size_t)) / sizeof(record_t_int))
#define TREE_ORDER_str  ((BLOCK_SIZE - 3 * sizeof(off_t) - sizeof(size_t)) / sizeof(record_t_str))
// =====================ReadBlock and WriteBlock are not my work ===============
// @NOTE here we need to invoke Buffer module to read / write blocks
void *ReadBlock(char *fileName, off_t offset, size_t size);  // return a pointer which points to a block in memory
int WriteBlock(char *fileName, void *block, off_t offset, size_t size);  // return 1 if succeeded or 0 if not
// =============================================================================
// key and value definition
typedef off_t value_t;  // value type, default int

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
    char path[256];  // path to the index file
    meta_t meta;  // meta data
};

typedef struct my_key_t_int my_key_t_int;  // key type (int, float, varchar)
struct my_key_t_int
{
    int key;
    //float key;
    //char key[256];
};

typedef struct my_key_t_float my_key_t_float;  // key type (int, float, varchar)
struct my_key_t_float
{
    //int key;
    float key;
    //char key[256];
};

typedef struct my_key_t_str my_key_t_str;  // key type (int, float, varchar)
struct my_key_t_str
{
    //int key;
    //float key;
    char key[256];
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

typedef struct record_t_int record_t_int;
struct record_t_int
{
    my_key_t_int key;
    value_t value;
};

typedef struct leaf_t_int leaf_t_int;
struct leaf_t_int
{
    off_t parent;
    off_t next;
    off_t prev;
    size_t n;
    record_t_int children[TREE_ORDER_int];
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
// ============= other modules can invoke the following functions ==============
void InitTree(BPlusTree tree, char *path, enum DataType type);
// =============================================================================
int IntKeyCmp(int A, int B);
int FloatKeyCmp(float A, float B);
int StringKeyCmp(char *A, char *B);
void UnallocLeaf(BPlusTree tree);
void UnallocInternal(BPlusTree tree);

#endif
