#ident	"@(#)Makefile	11.2"

LOCALINC =	-I..

include		$(CMDRULES)

LDLIBS =	-L$(USRLIB) ../c-client/libc-client.a -lgen -lsocket

OBJS =		mailcheck.o callbacks.o

DEST =		mailcheck

$(DEST):	$(OBJS
		$(CC) $(CFLAGS) $(DEFLIST) -o $(DEST) $(OBJS) $(LDFLAGS) $(LDLIBS)

build:		$(DEST)

callbacks.o:    ../c-client/callbacks.c
		$(CC) $(CFLAGS) $(DEFLIST) -c $?

install:	build
		if [ ! -d $(BIN) ] ; then mkdir $(BIN) ; fi
		$(INS) -f $(BIN) mailcheck

local-install:
		cp mailcheck /bin

local-remove:
		rm -f /bin/mailcheck

clean:
		rm -f $(OBJS)

clobber:	clean
		rm -f $(DEST)
