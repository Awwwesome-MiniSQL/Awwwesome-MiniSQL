### Install Guide安装向导
#### Requirements
* ncurses library
* readline library (for linux)
* gcc 4.9+

``` bash
$ apt-get install ncurses-dev
$ wget -c http://chenyuan.me/readline-master.tar.gz
$ tar -zxvf readline-master.tar.gz
$ cd readline-master
$ ./configure
$ make && make install
```
#### Compile MiniSQL and Run
``` bash
$ make
$ ./MinniSQL
```
### MiniSQL
This MiniSQL project is maintained by [谢嘉豪](http://10.214.224.77:81/u/xjiajiahao), [陈源](http://10.214.224.77:81/u/chenyuan) and [张扬光](http://10.214.224.77:81/u/Yangguang.Zhang).

#### Overview
MiniSQL supports the following data types for an attribute:
* int
* float
* char(n), 1 <= n <= 255

MiniSQL supports the following standard SQL statements.  
Note that all SQL statements should **END WITH `;`** and **key words should be lowercase**.
* create / drop table  
```sql
create table tableName (attrA int, attrB float unique, attrC char(20), primary key(attrX));
drop table tableName;
```

* create / drop index
```sql
create index indexName on tableName(attrName);
drop index indexName;
```

* select
```sql
select attrA, attrB from tableName where attrC = X;
select * from tableName;
```

* insert
```sql
insert into tableName (attrA, attrB, attrC) values (valueA, valueB, valueC);
```

* delete
```sql
delete from tableName where attrA >= X and attrB < Y;
```
* quit

* import SQL file
```sql
source fileName.sql
```

#### Detailed Specification
MiniSQL consists of seven components:
* `Interpreter`  
Interpret SQL statements, invoke API and return results.

* `API`   
Provide APIs that execute SQL statements, connect `Interpreter` with other components.

* `Record Manager`  
Create / delete files, insert / delete / select tuples.

* `Index Manager`  
Create / delete B+ trees, insert / delete / select keys.

* `Catalog Manager`  
Access and manipulate meta-data of tables, attributes and indexes.

* `Buffer Manager`  
Read / write blocks, buffer replacement, record status and locks.

* `DB Files`  
Catalog / data / index files.

#### Duty Assignments
* 陈源: `Interpreter`, `API` **Deadline: June 7, 2016**  

* 张扬光: `Buffer Manager`, `Catalog Manager` **Deadline: June 7, 2016**

* 谢嘉豪: `Index Manager`, `Record Manager` **Deadline: June 7, 2016**
