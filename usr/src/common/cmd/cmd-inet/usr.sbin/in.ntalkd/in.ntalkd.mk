#ident	"@(#)in.ntalkd.mk	1.2"
#ident	"$Header$"

#
#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.
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
# 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
# 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
#       (c) 1990,1991  UNIX System Laboratories, Inc.
# 	          All rights reserved.
#  
#

include $(CMDRULES)

LOCALDEF=	-DSYSV 
INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

LDLIBS=		-lsocket -lnsl -lgen

OBJS=		in.talkd.o announce.o process.o table.o print.o

all:		in.talkd

in.talkd:	$(OBJS)
		$(CC) -o in.talkd $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) in.talkd

clean:
		rm -f $(OBJS)

clobber:	clean
		rm -f in.talkd

lintit:
		$(LINT) $(LINTFLAGS) *.c

FRC:

#
# Header dependencies
#

announce.o:	announce.c \
		$(INC)/protocols/talkd.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/signal.h \
		$(INC)/termio.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/time.h \
		$(INC)/time.h \
		$(INC)/stdio.h \
		$(INC)/sys/wait.h \
		$(INC)/errno.h \
		$(INC)/syslog.h \
		$(FRC)

in.talkd.o:	in.talkd.c \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/signal.h \
		$(INC)/syslog.h \
		$(INC)/protocols/talkd.h \
		$(FRC)

print.o:	print.c \
		$(INC)/protocols/talkd.h \
		$(INC)/stdio.h \
		$(INC)/syslog.h \
		$(INC)/netinet/in.h \
		$(FRC)

process.o:	process.c \
		$(INC)/protocols/talkd.h \
		$(INC)/stdio.h \
		$(INC)/syslog.h \
		$(INC)/netdb.h \
		$(FRC)

table.o:	table.c \
		$(INC)/stdio.h \
		$(INC)/sys/time.h \
		$(INC)/syslog.h \
		$(INC)/protocols/talkd.h \
		$(INC)/netinet/in.h \
		$(FRC)
