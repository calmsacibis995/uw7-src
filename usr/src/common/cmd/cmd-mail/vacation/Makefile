#ident	"@(#)Makefile	11.2"

include 	$(CMDRULES)

LDLIBS =	-L$(USRLIB) -lgen

build:		vacation

vacation:	vacation.c Makefile
		$(CC) $(CFLAGS) $(DEFLIST) -o vacation vacation.c $(LDFLAGS) $(LDLIBS)

# no LDFLAGS to get rid of -s.
vacationt:	vacation.c Makefile
		$(CC) -ql -o vacationt -DDEBUG $(DEFLIST) vacation.c $(LDLIBS)

install:	build
		if [ ! -d $(BIN) ] ; then mkdir -p $(BIN) ; fi
		$(INS) -f $(BIN) vacation

local-install:
		cp vacation /usr/bin/vacation

local-remove:
		rm -f /usr/bin/vacation

clean:

clobber:
		rm -f vacation vacationt
