#ident	"@(#)Makefile	11.1"

SHELL	= /bin/sh

LOCALDEF=	-I. -Dconst= -DSTDPROTO=bezerkproto \
		-DMAILSPOOL=\"/var/mail\" \
		-DACTIVEFILE=\"/usr/share/news/active\" \
		-DNEWSSPOOL=\"/var/spool/news\" \
		-DRSH=\"rsh\" \
		-DRSHPATH=\"/usr/lib/rsh\" \
		-DSCOMS

include $(CMDRULES)

LIB =		libclient.a

build:		$(LIB)

OBJS =		bezerk.o \
		dummy.o \
		imap2.o \
		mail.o \
		mbox.o \
		misc.o \
		mmdf.o \
		news.o \
		newsrc.o \
		nntp.o \
		nntpcunx.o \
		os_sv4.o \
		rfc822.o \
		sm_unix.o \
		smtp.o \
		scomsc1init.o

$(LIB):		linkage.c $(OBJS)
		$(AR) ruv $(LIB) $(OBJS)

scomsc1init.o:	../../c-client/scomsc1init.o
		cp ../../c-client/scomsc1init.o .

$(OBJS):	linkage.h

clean:
		rm -f $(OBJS) linkage.c linkage.h

clobber:	clean
		rm -f $(LIB)

linkage.h linkage.c:
		sh drivers imap nntp mmdf bezerk news dummy
