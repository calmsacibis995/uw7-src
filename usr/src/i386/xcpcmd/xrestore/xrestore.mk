#ident	"@(#)xrestore:xrestore.mk	1.3.3.1"

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved

#	This Module contains Proprietary Information of Microsoft
#	Corporation and should be treated as Confidential.

#
#	@(#) xrestore.mk 1.3 88/05/16 xrestore:xrestore.mk
#

include	$(CMDRULES)

LDLIBS	= -lcmd

# objects
EXES	= xrestor
OBJS	= restor.o
SRCS	= restor.c
LOCALDEF = -D_KMEMUSER

# standard targets
all :	exes libs

install: all
	$(INS) -f $(USRBIN) -m 555 -u bin -g bin xrestor 
	-rm -f $(USRBIN)/xrestore
	ln $(USRBIN)/xrestor $(USRBIN)/xrestore
	cp restor.dfl $(ETC)/default/xrestor

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(EXES)

FRC:

# application targets
$(OBJS):	$(FRC)

exes:	$(EXES)

libs:	restor.dfl


xrestor:		restor.o
	$(CC) -o xrestor restor.o $(LDFLAGS) $(LDLIBS) 
