# You need to make sure these paths are correct
PSQLINC = /usr/include/postgresql
PSQLLIB = /usr/lib/postgresql/lib

# You shouldn't have to change anything under this line
SHELL = /bin/sh

TARGET1 = mdebs

SRCS = \
	dbinit.c \
	delete.c \
	dump.c \
	generalized.c \
	insert.c \
	journal.c \
	main.c \
	mdebs.y \
	mdebs.yy \
	messages.c \
	optproc.c \
	query.c \
	version.c 

OBJECTS1 = \
	dbinit.o \
	delete.o \
	dump.o \
	generalized.o \
	insert.o \
	journal.o \
	main.o \
	messages.o \
	optproc.o \
	../pgresfunc/pgresfunc.o \
	query.o \
	version.o \
	y.tab.o \
	lex.yy.o 

OBJECTS2 = \
	y.tab.c \
	y.tab.h \
	lex.yy.c

CC = gcc
CCFLAGS = -I$(PSQLINC)
LD = gcc
LDFLAGS= -L$(PSQLLIB)
LDLIBS = -lpq -lreadline -lcrypt

all : $(TARGET1) tags docs

$(TARGET1) : $(OBJECTS1)
	$(LD) $(LDFLAGS) -o $(TARGET1) $(OBJECTS1) $(LDLIBS)

convert : mdebs.h convert.c ../pgresfunc/pgresfunc.o generalized.o messages.o
	$(CC) $(CCFLAGS) -c convert.c -o convert.o
	$(LD) $(LDFLAGS) -o convert pgresfunc.o convert.o generalized.o messages.o $(LDLIBS)

dbinit.o : dbinit.h dbinit.c
	$(CC) $(CCFLAGS) -c dbinit.c -o dbinit.o

delete.o : delete.h delete.c
	$(CC) $(CCFLAGS) -c delete.c -o delete.o

dump.o : printouts.h dump.h dump.c
	$(CC) $(CCFLAGS) -c dump.c -o dump.o

generalized.o : generalized.h generalized.c
	$(CC) $(CCFLAGS) -c generalized.c -o generalized.o

insert.o : insert.h insert.c
	$(CC) $(CCFLAGS) -c insert.c -o insert.o

journal.o : journal.h journal.c
	$(CC) $(CCFLAGS) -c journal.c -o journal.o

main.o : mdebs.h y.tab.h main.c
	$(CC) $(CCFLAGS) -c main.c -o main.o

messages.o : messages.h messages.c
	$(CC) $(CCFLAGS) -c messages.c -o messages.o

optproc.o : optproc.h optproc.c
	$(CC) $(CCFLAGS) -c optproc.c -o optproc.o

query.o : query.h query.c
	$(CC) $(CCFLAGS) -c query.c -o query.o

version.o : version.c
	$(CC) $(CCFLAGS) -c version.c -o version.o

y.tab.o : y.tab.h y.tab.c
	$(CC) $(CCFLAGS) -c y.tab.c -o y.tab.o

y.tab.c y.tab.h : mdebs.y
	bison -d -y mdebs.y

lex.yy.o : lex.yy.c
	$(CC) $(CCFLAGS) -c lex.yy.c -o lex.yy.o

lex.yy.c : mdebs.yy y.tab.h
	flex -I mdebs.yy

tags :	
	ctags $(SRCS)

.PHONY :	docs
docs : 
	$(MAKE) -C doc

.PHONY :	clean 
clean :
	rm -f $(OBJECTS1) $(OBJECTS2) *~ core *.output \
	*.backup *.bak tags

.PHONY : 	docclean
docclean :
	$(MAKE) -C doc clean

.PHONY :	dist-clean
distclean :
	rm -f mdebs $(OBJECTS1) $(OBJECTS2) *~ core *.output *.backup *.bak


