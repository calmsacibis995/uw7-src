#ident	"@(#)Makefile	11.2

include		../Make.defs

LOCALINC =	-I../src $(DEFS)

include		$(CMDRULES)

LDLIBS =	-L$(USRLIB)

mailstats:	mailstats.c
		$(CC) -o mailstats $(CFLAGS) $(DEFLIST) mailstats.c $(LDFLAGS) $(LDLIBS)

build:		mailstats

clean clobber:
		rm -f mailstats
