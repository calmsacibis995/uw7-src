#ident	"@(#)Makefile	11.1"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# @(#)Makefile	1.5
# Copyright (c) 1991 Bell Communications Research, Inc. (Bellcore)
# 
# Permission to use, copy, modify, and distribute this material 
# for any purpose and without fee is hereby granted, provided 
# that the above copyright notice and this permission notice 
# appear in all copies, and that the name of Bellcore not be 
# used in advertising or publicity pertaining to this 
# material without the specific, prior written permission 
# of an authorized representative of Bellcore.  BELLCORE 
# MAKES NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY 
# OF THIS MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS", 
# WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES.
#

VERS=2.3.6

CONFIGDIR = ..
LOCALINC= -I. -I${CONFIGDIR}
LOCALDEF=
#CFLAGS = -O $(LOCALINC) $(LOCALDEF)

include $(CMDRULES)

LDLIBS = -L$(USRLIB)

SHELL=/bin/sh

RICHOBJS=richtext.o richlex.o richset.o usascii.o iso2022.o

all:  richtext

richtext: $(RICHOBJS)
	$(CC) ${CFLAGS} -o richtext richtext.o $(RICHOBJS) $(LDLIBS) -lcurses $(LDFLAGS)

clean:
	rm -f $(RICHOBJS)

clobber: clean
	rm -f richtext

tar:
	tar cvf richtext-$(VERS).tar *.c *.h richtext.1 Makefile COMMANDS NOTES
