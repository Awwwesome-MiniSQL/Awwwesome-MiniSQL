#ifndef BPlusTree_H
#define BPlusTree_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#define Tree_ORDER  100

int ReadBlock(FILE *fp, off_t offset);
int WriteBlock(FILE *fp, void *block, off_t offset, size_t size);
typedef int value_t;  // value type, default int
typedef struct my_key_t my_key_t;  // key type, default int
struct my_key_t
{
    char key[256];
};

typedef struct record_t record_t;
struct record_t
{
    key_t key;
    value_t value;
};

typedef struct leaf_t leaf_t;
struct leaf_t
{

};

typedef struct internal_t internal_t;
struct internal_t
{

};

typedef struct meta_t meta_t;
struct meta_t
{

};

typedef struct index_t index_t;
struct index_t
{

};

typedef struct tree_t *BPlusTree;
struct tree_t
{
    FILE *fp;
};

int Insert(BPlusTree tree, my_key_t key, value_t value);
int Search(BPlusTree tree, my_key_t key, value_t *value);
int Remove(BPlusTree tree, my_key_t key);
int Update(BPlusTree tree, my_key_t key, value_t value);

#endif
