#ident	"@(#)Makefile	11.1

include		$(CMDRULES)

LDLIBS =	-L$(USRLIB)

execmail:	execmail.c
		$(CC) -o execmail $(CFLAGS) $(DEFLIST) execmail.c $(LDFLAGS) $(LDLIBS)

build:		execmail

clean clobber:
		rm -f execmail
