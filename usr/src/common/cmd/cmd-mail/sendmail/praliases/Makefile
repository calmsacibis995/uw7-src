#ident	"@(#)Makefile	11.1

include		../Make.defs

LOCALINC =	-I../src -I../../libdb/include $(DEFS)

include		$(CMDRULES)

LDLIBS =	-L../../libdb -L$(USRLIB) -ldb

praliases:	praliases.c
		$(CC) -o praliases $(CFLAGS) $(DEFLIST) praliases.c $(LDFLAGS) $(LDLIBS)

build:		praliases

clean clobber:
		rm -f praliases
