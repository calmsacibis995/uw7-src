#	copyright	"%c%"

#ident	"@(#)fold:fold.mk	1.2.4.4"

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#


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
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.

#     Makefile for fold

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

#top#

MAKEFILE = fold.mk

MAINS = fold 

OBJECTS =  fold.o

SOURCES = fold.c 

LIBW=-lw

all:          $(MAINS)

$(MAINS):	fold.o
	$(CC) -o $@ fold.o $(LDFLAGS) $(LDLIBS) $(SHLIBS) $(LIBW)
	
fold.o:		$(INC)/stdio.h 	\
		$(INC)/sys/types.h	\
		$(INC)/sys/euc.h	\
		$(INC)/stdlib.h	\
		$(INC)/errno.h	\
		$(INC)/string.h	\
		$(INC)/getwidth.h	\
		$(INC)/locale.h	\
		$(INC)/pfmt.h


GLOBALINCS = $(INC)/stdio.h 

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

install:	all
	$(INS) -f $(INSDIR) -m 555 -u $(OWN) -g $(GRP) fold 

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

#     These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(INSDIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
	sed 's;^;$(INSDIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)

