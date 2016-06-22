create table person (
	pid int primary key,
	name char(32),
	identity int,
	age int,
	height float,
	weight float,
	address char(64)
);

drop table person;
