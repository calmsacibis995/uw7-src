#ident	"@(#)Makefile	11.2"

SHELL =		/bin/sh

LOCALINC =	-I../c-client -I../mhome/lib
LOCALDEF =

include		$(CMDRULES)

LDLIBS =	-L$(USRLIB) ../c-client/libc-client.a \
		../mhome/lib/libmultihome.a ../libdb/libdb.a \
		-lgen -lsocket

OBJS =		popper.o mhome.o

build:		popper

popper:		$(OBJS) ../c-client/libc-client.a
		$(CC) $(CFLAGS) $(DEFLIST) -o popper $(OBJS) $(LDFLAGS) $(LDLIBS)

mhome.o:	../imap/mhome.c
		$(CC) $(CFLAGS) $(DEFLIST) -c ../imap/mhome.c

install:
		if [ ! -d $(ETC) ] ; then mkdir -p $(ETC) ; fi
		$(INS) -f $(ETC) popper

local-install:
		cp popper /etc/popper
	echo services:   popper		110/tcp				# POP
	echo inetd.conf: popper	stream	tcp	nowait	root	/etc/popper	popper

local-remove:
		rm -f /etc/popper

clean:
		rm -f $(OBJS)

clobber:	clean
		rm -f popper

