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
#define InsertIndex(a, b, c) _Generic(b, my_key_t_int: Insert_int, float: Insert_float, char *: Insert_str)(a, b, c)
#define SearchIndex(a, b) _Generic(b, my_key_t_int: Search_int, float: Search_float, char *: Search_str)(a, b)
#define RemoveIndex(a, b) _Generic(b, my_key_t_int: Remove_int, float: Remove_float, char *: Remove_str)(a, b)
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
    char path[1024];  // path to the index file
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

// ============= other modules can invoke the following functions ==============
void InitTree(BPlusTree tree, char *path, enum DataType type);
// =============================================================================
int IntKeyCmp(int A, int B);
int FloatKeyCmp(float A, float B);
int StringKeyCmp(char *A, char *B);
void UnallocLeaf(BPlusTree tree);
void UnallocInternal(BPlusTree tree);

#endif
