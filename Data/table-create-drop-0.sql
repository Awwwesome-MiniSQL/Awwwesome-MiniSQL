create table person (
	height float,
	pid int,
	name char(32),
	identity char(64),
	age int unique,
	primary key(pid)
);

drop table person;
