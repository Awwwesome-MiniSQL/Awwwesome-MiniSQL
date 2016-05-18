#ifndef MINISQL_H
#define MINISQL_H
#define MaxAttrNum 32
// three types of data, int, float, and string (the size of a string is between 0 and 255)
enum DataType{intType, floatType, stringType};
typedef struct AttributeRecord *Attribute
struct AttributeRecord
{
    char *name;
    DataType type;
    char size;  // the size of the attribute
    char unique;  // unique == 1 (the attribute is unique) or 0 (not unique)
    char primary; // maybe unnecessary, primary == 1 (the primary key) or 0 (not primary key)
    int index;  // index name
};

typedef struct TableRecord *Table;
struct TableRecord
{
    char *name;
    struct AttributeRecord column[MaxAttrNum];
    int primaryKey;  // the index of primary key
    int recordSize;  // the size of a tuple
    int buff;  // the location of the table in the buffer
    int recordNum;  // the number of records in the table
};

#endif
