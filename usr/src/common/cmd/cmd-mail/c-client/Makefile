#ident	"@(#)Makefile	11.2"

SHELL =		/bin/sh

LOCALDEF =	-DANONYMOUSHOME=\"/var/mail/anonymous\" \
		-DACTIVEFILE=\"/usr/share/news/active\" \
		-DMAILSPOOL=\"/var/mail\" \
		-DNEWSSPOOL=\"/var/spool/news\" \
		-DRSH=\"rsh\" \
		-DRSHPATH=\"/usr/lib/rsh\" \
		-DSTDPROTO=scoms1proto

include		$(LIBRULES)

LDLIBS =	-L$(USRLIB) -lgen -lsocket
SFLAGS =	-G -dy -h /usr/lib/libc-client.so

LIB =		libc-client.a
SLIB =		libc-client.so

OBJS =		mail.o os_sv4.o rfc822.o misc.o \
		smanager.o dummy.o news.o newsrc.o \
		imap4.o nntp.o netmsg.o scoms1.o scomsc1.o scomsc1init.o

build:		$(LIB) $(SLIB)

$(LIB):		$(OBJS)
		$(AR) rv $@ $?

$(SLIB):	$(OBJS)
		$(CC) $(SFLAGS) -o $(SLIB) $(OBJS) $(LDLIBS)

install:
		if [ ! -d $(USRLIB) ] ; then mkdir -p $(USRLIB) ; fi
		if [ ! -d $(ETC)/default ] ; then mkdir -p $(ETC)/default ; fi
		if [ ! -d $(ETC)/mail ] ; then mkdir -p $(ETC)/mail ; fi
		$(INS) -f $(USRLIB) $(LIB)
		$(INS) -f $(USRLIB) $(SLIB)
		$(INS) -f $(ETC)/default mail
		cp mail mail.factory
		$(INS) -f $(ETC)/mail mail.factory

local-install:
		if [ ! -d /etc/mail ] ; then mkdir -p /etc/mail ; fi
		if [ ! -d /etc/default ] ; then mkdir -p /etc/default ; fi
		cp $(LIB) $(SLIB) /usr/lib
		cp mail /etc/default
		cp mail /etc/mail/mail.factory

local-remove:
		rm -f /usr/lib/$(LIB) /usr/lib/$(SLIB)
		rm -f /etc/default/mail
		rm -f /etc/mail/mail.factory

clean:
		rm -f $(OBJS) mail.factory

clobber:	clean
		rm -f $(LIB) $(SLIB)

.PRECIOUS:	mail.c

mail.c:		sco.c
		touch mail.c
