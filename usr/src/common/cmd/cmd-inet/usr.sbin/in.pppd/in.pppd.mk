#ident	"@(#)in.pppd.mk	1.2"
#ident	"$Header$"
#ident	"/proj/tcp/usl/ppp-5.1/lcvs/usr/src/common/cmd/cmd-inet/usr.sbin/in.pppd/in.pppd.mk,v 1.5 1995/06/28 19:04:05 neil Exp"


# 	Copyrighted as an unpublished work.
# 	(c) Copyright 1987-1995 Legent Corporation
# 	All rights reserved.
#
# 	RESTRICTED RIGHTS
#
# 	These programs are supplied under a license.  They may be used,
# 	disclosed, and/or copied only as permitted under such license
# 	agreement.  Any copy must contain the above copyright notice and
# 	this restricted rights notice.  Use, copying, and/or disclosure
# 	of the programs is strictly prohibited unless otherwise provided
# 	in the license agreement.
#
#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

#
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# 		PROPRIETARY NOTICE (Combined)
# 
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
# 
# 
# 
# 		Copyright Notice 
# 
# Notice of copyright on this source code product does not indicate 
# publication.
# 

# 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
#       (c) 1990,1991  UNIX System Laboratories, Inc.
# 	          All rights reserved.
#  
#

include $(CMDRULES)

LEX=		/usr/bin/lex
YACC=		/usr/bin/yacc

LOCALDEF=	-DDOCHAP -DUNIXWARE -DSVR4 -DSYSV -D_DLPI -D_KMEMUSER -DSTATIC=
INSDIR=		$(USRSBIN)
LIBDIR=		$(USRLIB)/ppp
OWN=		bin
GRP=		bin

LDLIBS=		-lsocket -lnsl -lgen 
LDFLAGS=	-L $(USRLIB)

DOBJS=		in.pppd.o sock_supt.o gtphostent.o gtphstnamadr.o uucpspt.o \
	        gencode.o optimize.o tcpgram.o tcplex.o util.o	addr.o \
		pool.o proxy.o list.o md5.o

POBJS=		ppp.o sock_supt.o	

TOBJS=		pppstat.o sock_supt.o pool.o addr.o list.o

AOBJS=		pppattach.o sock_supt.o

all:		in.pppd ppp pppstat pppattach pppconf

in.pppd:	$(DOBJS)
		$(CC) -o in.pppd  $(LDFLAGS) $(DOBJS) $(LDLIBS) $(SHLIBS)

ppp:		$(POBJS)
		$(CC) -o ppp  $(LDFLAGS) $(POBJS) $(LDLIBS) $(SHLIBS)

pppstat:	$(TOBJS)
		$(CC) -o pppstat  $(LDFLAGS) $(TOBJS) $(LDLIBS) $(SHLIBS)

pppattach:	$(AOBJS)
		$(CC) -o pppattach  $(LDFLAGS) $(AOBJS) $(LDLIBS) $(SHLIBS)

pppconf:	pppconf.sh
		cp pppconf.sh pppconf

tcplex.c: tcplex.l
	rm -f $@
	$(LEX) $<
	mv -f lex.yy.c  tcplex.c

tokdefs.h: tcpgram.c

tcpgram.c: tcpgram.y
	rm -f tcpgram.c tokdefs.h
	$(YACC) -d $< 
	mv -f y.tab.c tcpgram.c
	mv -f y.tab.h tokdefs.h


install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) in.pppd
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) pppstat
		if [ ! -d $(LIBDIR) ] ; then mkdir  $(LIBDIR) 2>/dev/null ; fi
		$(INS) -f $(LIBDIR) -m 04711 -u root -g $(GRP) ppp
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) pppconf
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) pppattach

md5.c:	md5.h
	if [ -r ../../../../uts/net/md5/md5.c ] ;\
	then cp ../../../../uts/net/md5/md5.c . ;\
	else cp omd5.c md5.c ;\
	fi

md5.h:
	if [ -r ../../../../uts/net/md5/md5.h ] ;\
	then cp ../../../../uts/net/md5/md5.h . ;\
	else cp omd5.h md5.h ;\
	fi

clean:
		rm -f $(DOBJS) $(POBJS) $(TOBJS) tokdefs.h tcplex.c tcpgram.c

clobber:	clean
		rm -f in.pppd pppstat ppp 

FRC:

#
# Header dependencies
#

md5.o:	md5.c \
	md5.h \
	$(FRC)

in.pppd.o:	md5.h \
	$(FRC)
