#ident	"@(#)Makefile	11.1

include		../Make.defs

LOCALINC =	-I../src

include		$(CMDRULES)

LDLIBS =	-L$(USRLIB) -lgen

smrsh:		smrsh.c
		$(CC) -o smrsh $(CFLAGS) $(DEFLIST) smrsh.c $(LDFLAGS) $(LDLIBS)

build:		smrsh

clean clobber:
		rm -f smrsh
