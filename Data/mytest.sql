create table person (
	height float unique,
	pid int,
	name char(32),
	identity char(128) unique,
	age int unique,
	primary key(pid)
);
insert into person values (1300.0, 0, "Tom", "cf0a2386-2435-423f-86b8-814318bedbc7", 0);
