#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)renice:renice.mk	1.2.1.1"
#ident	"@(#)ucb:common/ucbcmd/renice/renice.mk	1.2"
#ident	"$Header$"


#		PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.

#     Makefile for renice

include $(CMDRULES)

INSDIR = $(USRSBIN)
OWN = bin
GRP = bin

MAKEFILE = renice.mk

MAINS = renice

OBJECTS =  renice.o setpriority.o

SOURCES = renice.c  setpriority.c

ALL:          $(MAINS)

$(MAINS):	renice.o setpriority.o
	$(CC) $(LDFLAGS) -o renice $(OBJECTS) $(LDLIBS) $(PERFLIBS)
	
renice.o:		$(INC)/stdio.h $(INC)/sys/time.h \
		$(INC)/sys/resource.h $(INC)/pwd.h \
		$(INC)/sys/types.h $(INC)/unistd.h \
		$(INC)/locale.h $(INC)/pfmt.h

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all :	ALL

install:	ALL
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) -m 555 $(MAINS)

