#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "MiniSQL.h"
#include "Record/Record.h"
#include "BPlusTree/BPlusTree.h"
#include "BPlusTree/BPlusTreeInt.h"
#include "BPlusTree/BPlusTreeFloat.h"
#include "BPlusTree/BPlusTreeStr.h"
#include "Catalog/Catalog.h"
#include "interpreter.c"

int main() {
    //TEST:
    //Not Implemented:Create Drop Index
    char sql1[]="create index name_index on student (name)";
    interpreter(sql1);
    puts("========================");

    char sql2[]="drop index name_index on student";
    interpreter(sql2);
    puts("========================");

    //Create Table
    char sql3[]="create table student (xh char(10) unique primary key,id int,name char(20),major char(30),GPA float);";
    interpreter(sql3);//this is preparing for call insert and select
    puts("========================");

    //Insert
    char sql4[]="insert into student values (\"3140105754\",1,\"Chen Yuan\",\"Biology\",3.55)";
    interpreter(sql4);
    puts("========================");

    //Select
    //char sql5[]="select xh,GPA from student where GPA>4 and id<5 and xh='314' and GPA<5";
    char sql5[]="select * from student";
    interpreter(sql5);
    puts("========================");

    //Delete
    char sql6[]="delete from student";// where GPA>4 and id<5 and xh='314' and GPA<5";
    interpreter(sql6);
    puts("========================");
    //interpreter("select * from student");
    //Drop
    /*
    char sql7[]="drop table student";
    interpreter(sql7);
    */
    puts("========================");
    return 0;
}
