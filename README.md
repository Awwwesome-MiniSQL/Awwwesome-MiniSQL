### Install Guide安装向导
#### Requirements
* ncurses library
* readline library (for linux)
* gcc 4.9+

#### Prepare, Download, Compile and Run
We need to install gcc-4.9 and readline to make our program.
``` bash
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install -y ncurses-dev libreadline6-dev gcc-4.9
sudo ln -s /usr/bin/gcc-4.9 /usr/bin/gcc -f
curl -o Awwwesome-MiniSQL.zip  https://codeload.github.com/Awwwesome-MiniSQL/Awwwesome-MiniSQL/zip/master
unzip Awwwesome-MiniSQL.zip
cd Awwwesome-MiniSQL
make
./MiniSQL
```
In MiniSQL, you can type:
```
exec Data/table-create-drop-0.sql;
exec Data/table-create-drop-1.sql;
exec Data/table-create-drop-2.sql;
exec Data/table-insert-delete-0.sql;
exec Data/table-insert-delete-1.sql;
exec Data/table-select-0.sql;
exec Data/table-select-1.sql;
exec Data/test-1000.sql;
exec Data/test-1w.sql;
exec Data/test-10w.sql;
select * from person where age = 380;
quit
```


### MiniSQL
This MiniSQL project is maintained by [Stephen Tse](https://github.com/xjiajiahao), [chenyuan]() and [Yangguang.Zhang]().

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
insert into tableName values (valueA, valueB, valueC);
```

* delete
```sql
delete from tableName where attrA >= X and attrB < Y;
```
* quit

* import SQL file
```sql
exec fileName.sql
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

