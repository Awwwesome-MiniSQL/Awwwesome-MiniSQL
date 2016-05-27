#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MiniSQL.h"
#include "BPlusTree.h"

int main()
{
    FILE *fp;
    meta_t *meta;
    leaf_t *leaf;
    internal_t *internal;
    my_key_t newKey;
    struct tree_t tree;
    char fileName[1024];
    scanf("%s", fileName);
    fp = fopen(fileName, "w");
    fclose(fp);
    // test InitTree
    InitTree(&tree, fileName);
    // test ReadBlock
    meta = (meta_t *)ReadBlock(fileName, META_OFFSET, sizeof(meta_t));
    printf("meta data:\n");
    printf("meta.order: %ld\nmeta.valueSize: %ld\nmeta.keySize: %ld\nmeta.internalNum: %ld\nmeta.leafNum: %ld\nmeta.height: %ld\nmeta.slot: %ld\nmeta.rootOffset: %ld\nmeta.leafOffset: %ld\n", meta->order, meta->valueSize, meta->keySize, meta->internalNum, meta->leafNum, meta->height, meta->slot, meta->rootOffset, meta->leafOffset);
    // test insert
    strcpy(newKey.key, "good bye");
    Insert(&tree, newKey, 0x12345678);
    leaf = (leaf_t *)ReadBlock(fileName, meta->leafOffset, sizeof(leaf_t));
    printf("leaf data:\n");
    printf("leaf->parent: %ld\nleaf->next: %ld\nleaf->prev: %ld\nleaf->n: %ld\nleaf->children[0].value: %u\nleaf->children[0].key.key: %s\n", leaf->parent, leaf->next, leaf->prev, leaf->n, leaf->children[0].value, leaf->children[0].key.key);
    return 0;
}
