#ident	"@(#)Makefile	11.2

include		../Make.defs

LOCALINC =	-I../src -I../../libdb -I../../libdb/hash \
		-D_PATH_SENDMAIL=\"/usr/lib/sendmail\"

include		$(CMDRULES)

LDLIBS =	-L../../libdb -L$(USRLIB) -ldb

rmail:		rmail.c
		$(CC) -o rmail $(CFLAGS) $(DEFLIST) rmail.c $(LDFLAGS) $(LDLIBS)

build:		rmail

clean clobber:
		rm -f rmail
