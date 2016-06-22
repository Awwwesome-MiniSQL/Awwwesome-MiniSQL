#ifndef MINISQL_H
#define MINISQL_H
#define MAX_ATTRIBUTE_NUM 32
#define BLOCK_SIZE 4096 // 4 KB a block
#define MAX_NAME_LENGTH 32
#define MAX_STRING_LENGTH 256
#define NOBUFFER
#define DEBUG
// three types of data, int, float, and string (the size of a string is between 0 and 255)
enum DataType{intType, floatType, stringType};
struct AttributeRecord
{
    char name[MAX_NAME_LENGTH];  // the length of the name of attributes should be less than 16
    enum DataType type;
    int size;  // the size of the attribute, @NOTE '\0' should be counted
    char unique;  // unique == 1 (the attribute is unique) or 0 (not unique)
    int index;  // index name
    char indexName[MAX_NAME_LENGTH]; // the new one for users
};

typedef struct TableRecord *Table;
struct TableRecord
{
    char name[MAX_NAME_LENGTH];

    int recordSize;  // the size of a tuple
    int recordNum;  // the number of records in the table
    int recordsPerBlock;  // the number of blocks of records in the file

    int primaryKey;  // the index of primary key
    int attrNum;
    struct AttributeRecord attributes[MAX_ATTRIBUTE_NUM];
};

extern char FLAG_RECORD_INFO;
#endif
