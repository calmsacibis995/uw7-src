#ident "@(#)Makefile	11.1"

SHELL =		/bin/sh

LIB =		../../mhome/lib/libmultihome.a ../../libdb/libdb.a
INC =		-I../../mhome/lib

libtest:	libtest.c $(LIB)
		cc $(INC) -o libtest libtest.c $(LIB)

clobber clean:
		rm -f libtest.o libtest
