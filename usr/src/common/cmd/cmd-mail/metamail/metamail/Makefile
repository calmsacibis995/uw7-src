#ident	"@(#)Makefile	11.1"

LOCALINC= -I. -I..
LOCALDEF=
CFLAGS = $(LOCALINC) $(LOCALDEF)

include $(CMDRULES)

LDLIBS = -L$(USRLIB)

SHELL=/bin/sh

all: metamail mimencode mailto splitmail

lintit: mimencode.lint mailto.lint splitmail.lint metamail.lint

mimencode: mmencode.o codes.o
	$(CC) $(CFLAGS) -o mimencode  mmencode.o  codes.o $(LDFLAGS) $(LDLIBS)


mailto: mailto.o codes.o shared.o
	$(CC) $(CFLAGS) -o mailto  mailto.o  codes.o shared.o -lcurses $(LDFLAGS) $(LDLIBS)

splitmail: splitmail.o shared.o
	$(CC) $(CFLAGS) -o splitmail  splitmail.o shared.o $(LDFLAGS) $(LDLIBS)

metamail:  metamail.o codes.o uue.o shared.o
	$(CC) $(CFLAGS) -o metamail  metamail.o  codes.o uue.o shared.o $(LDFLAGS) $(LDLIBS)

clobber: clean
	rm -f mimencode mimencode.lint mailto mailto.lint splitmail splitmail.lint metamail metamail.lint

clean:
	rm -f mmencode.o codes.o splitmail.o shared.o mailto.o codes.o shared.o metamail.o  codes.o uue.o shared.o
