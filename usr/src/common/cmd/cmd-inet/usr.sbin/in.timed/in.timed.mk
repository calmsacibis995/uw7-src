#ident	"@(#)in.timed.mk	1.2"
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

LOCALDEF=	-DSYSV -DSTRNET
INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

LDLIBS=		-lsocket -lnsl -lresolv -lgen

TIMED_OBJS=	acksend.o candidate.o correct.o master.o netdelta.o \
		readmsg.o slave.o timed.o

TIMEDC_OBJS=	cmds.o cmdtab.o timedc.o

NETDATE_OBJS=	netdate.o

COM_OBJS=	byteorder.o measure.o cksum.o

all:		in.timed timedc netdate

in.timed:	$(TIMED_OBJS) $(COM_OBJS)
		$(CC) -o in.timed  $(LDFLAGS) $(TIMED_OBJS) $(COM_OBJS) \
		$(LDLIBS) $(SHLIBS)

timedc:		$(TIMEDC_OBJS) $(COM_OBJS)
		$(CC) -o timedc  $(LDFLAGS) $(TIMEDC_OBJS) $(COM_OBJS) \
		$(LDLIBS) $(SHLIBS)

netdate:	$(NETDATE_OBJS)
		$(CC) -o netdate  $(LDFLAGS) $(NETDATE_OBJS) \
		$(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) in.timed
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) timedc
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) netdate

clean:
		rm -f $(TIMED_OBJS) $(TIMEDC_OBJS) $(NETDATE_OBJS) $(COM_OBJS)

clobber:	clean
		rm -f in.timed timedc netdate

lintit:
		$(LINT) $(LINTFLAGS) *.c

FRC:
