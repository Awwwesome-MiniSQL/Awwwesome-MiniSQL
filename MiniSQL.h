#ifndef MINISQL_H
#define MINISQL_H
#define MAX_ATTRIBUTE_NUM 32
#define BLOCK_SIZE 4096 // 4 KB a block
#define DEBUG
// three types of data, int, float, and string (the size of a string is between 0 and 255)
enum DataType{intType, floatType, stringType};
struct AttributeRecord
{
    char name[16];  // the length of the name of attributes should be less than 16
    enum DataType type;
    char size;  // the size of the attribute
    char unique;  // unique == 1 (the attribute is unique) or 0 (not unique)
    int index;  // index name
};

typedef struct TableRecord *Table;
struct TableRecord
{
    char *name;
    struct AttributeRecord attributes[MAX_ATTRIBUTE_NUM];
    int primaryKey;  // the index of primary key
    int recordSize;  // the size of a tuple
    int buff;  // the location of the table in the buffer
    int recordNum;  // the number of records in the table
};

#endif
