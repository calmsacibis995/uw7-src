#
#	ident @(#) Makefile 11.2 97/10/30 
#
#############################################################################
#
#	Copyright (c) 1997 The Santa Cruz Operation, Inc.. 
#		All Rights Reserved. 
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF 
#		THE SANTA CRUZ OPERATION INC.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.
#
#############################################################################
#
#
# Modification History
#
# M000, 08-Sep-97, andrean
#	- Added message catalog targets.
#
#

SHELL	= /bin/sh

# must define INTL for i18n
LOCALDEF=	-I../.. -DINTL

LIBS	=	

include $(CMDRULES)

KSH =		/bin/ksh
MKCATDEFS =	$(TOOLS)/usr/bin/mkcatdefs
DONLS =		INS=$(INS) \
		MKCATDEFS=$(MKCATDEFS) \
		USRLIB=$(USRLIB) ETC=$(ETC) \
		$(KSH) ../../bin/donls

LIB	=	../../c-client/libc-client.a
LDLIBS	=	-L$(USRLIB) $(LIB) -lgen -lsocket

DESTDIR= $(USRBIN)

build:	mfck nls

OBJS =	mfck.o callbacks.o

mfck:	mfck_msg.h $(OBJS) Makefile $(LIB)
	$(CC) $(LDFLAGS) -o mfck $(OBJS) $(LDLIBS) $(LIBS)

# statement coverage instrumented version, must remove -s from LDFLAGS
mfckt:	mfck_msg.h mfck.c callbacks.o Makefile $(LIB)
	$(CC) -ql -o mfckt $(DEFLIST) mfck.c callbacks.o $(LDLIBS) $(LIBS)

mfck_msg.h:	NLS/en/mfck.msg
	$(MKCATDEFS) -a mail mfck NLS/en/mfck.msg >/dev/null

callbacks.o:    ../../c-client/callbacks.c
	$(CC) $(CFLAGS) $(DEFLIST) -c $?

nls:
	$(DONLS) -d NLS build

clean:
	rm -f $(OBJS)
	rm -f mfck_msg.h
	$(DONLS) -d NLS clean

clobber: clean
	-rm -f mfck mfckt
