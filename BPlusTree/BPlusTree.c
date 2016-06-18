#include <string.h>
#include "BPlusTree.h"
#include "../MiniSQL.h"
// ======================= buffer read and write ===============================
// ReadBlock and WriteBlock functions are provided by Buffer module
#ifdef NOBUFFER
void *ReadBlock(char *fileName, off_t offset, size_t size)
{
    void *block;
    FILE *fp;
    int n;
    block = (void *)malloc(size);  // @NOTE don't forget to free space later
    fp = fopen(fileName, "rb+");
    if (NULL == fp)
    {
        return NULL;
    }
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
// =============================================================================

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

void InitTree(BPlusTree tree, char *path, enum DataType type)
{
    switch (type) {
        case intType: InitTree_int(tree, path, type); break;
        case floatType: InitTree_float(tree, path, type); break;
        case stringType: InitTree_str(tree, path, type); break;
    }
}
