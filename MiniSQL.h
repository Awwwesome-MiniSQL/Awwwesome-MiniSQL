#ifndef MINISQL_H
#define MINISQL_H
#define MaxAttributeNum 32
// three types of data, int, float, and string (the size of a string is between 0 and 255)
enum DataType{intType, floatType, stringType};
struct AttributeRecord
{
    char *name;
    enum DataType type;
    char size;  // the size of the attribute
    char unique;  // unique == 1 (the attribute is unique) or 0 (not unique)
    int index;  // index name
};

typedef struct TableRecord *Table;
struct TableRecord
{
    char *name;
    struct AttributeRecord attributes[MaxAttributeNum];
    int primaryKey;  // the index of primary key
    int recordSize;  // the size of a tuple
    int buff;  // the location of the table in the buffer
    int recordNum;  // the number of records in the table
};

#endif
