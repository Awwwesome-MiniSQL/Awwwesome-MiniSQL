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

void UnallocLeaf(BPlusTree tree)
{
    tree->meta.leafNum--;
}

void UnallocInternal(BPlusTree tree)
{
    tree->meta.internalNum--;
}
