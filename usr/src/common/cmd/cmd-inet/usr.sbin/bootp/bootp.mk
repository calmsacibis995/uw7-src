#ident	"@(#)bootp.mk	1.2"
#ident	"$Header$"

#
# Copyrighted as an unpublished work.
# (c) Copyright 1987-1994 Lachman Technology, Inc.
# All rights reserved.
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

MORECPP =	-O

CFLAGS= $(MORECPP)
LOCALDEF=	-DSVR4 -DSYSV -DSTRNET -DBSD_COMP -D_KMEMUSER $(MORECPP)

INSDIR=         $(USRSBIN)
OWN=            bin
GRP=            bin

LDLIBS=		-lsocket -lnsl -lresolv

OBJS=		bootp.o

all:		bootp

bootp:		$(OBJS)
		$(CC) -o bootp $(LOCALDEF) $(LDFLAGS) $(OBJS) $(LDLIBS) \
		$(OBJS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) bootp

clean:
		rm -f $(OBJS) a.out core errs

clobber:	clean
		rm -f bootp

FRC:

#
# Header dependencies
#
