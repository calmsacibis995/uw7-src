#ident	"@(#)Makefile	11.2"

SHELL =		/bin/sh

LOCALINC =	-I../c-client -I../mhome/lib
LOCALDEF =	-DANOFILE=\"/etc/anonymous/newsgroups\" \
		-DALERTFILE=\"/etc/imapd.alert\"

include		$(CMDRULES)

LDLIBS =	-L$(USRLIB) ../c-client/libc-client.a \
		../mhome/lib/libmultihome.a ../libdb/libdb.a \
		-lpmapi -lgen -lsocket

OBJS =		imapd.o mhome.o spawn.o

build:		imapd

imapd:		$(OBJS) ../c-client/libc-client.a
		$(C++C) $(CFLAGS) $(DEFLIST) -o imapd $(OBJS) $(LDFLAGS) $(LDLIBS)

install:
		if [ ! -d $(ETC) ] ; then mkdir -p $(ETC) ; fi
		$(INS) -f $(ETC) imapd

local-install:
		cp imapd /etc/imapd
	echo services:   imap		143/tcp				# IMAP
	echo inetd.conf: imap	stream	tcp	nowait	root	/etc/imapd	imapd

local-remove:
		rm -f /etc/imapd

clean:
		rm -f $(OBJS)

clobber:	clean
		rm -f imapd

