#ident "@(#)Makefile	3.1"
#ident "$Header$"
#
# Makefile for eXample Network Media Support driver
#
#	Copyright (C) The Santa Cruz Operation, 1993-1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.

#
# Since during debug we alter <sys/dlpimod.h>, INCPATH points
# to the modified one rather than /usr/include.
INCPATH=-I../../../..	# For work on lliver.pdev.sco.com
#INCPATH=-I.		# For standalone driver

SHELL =		/bin/sh
#DEBUG =	-DDEBUG
CFLAGS =	 -O -w3 -wx $(INCPATH) -D_INKERNEL $(DEBUG)
OBJS =		xnms.o
BIN =		Driver.o
RM =		rm -f
LD =		ld

all:	$(BIN)

clean:
	$(RM) $(OBJS)

clobber:	clean
	$(RM) $(BIN)

Driver.o: $(OBJS)
	$(LD) -r $(OBJS) -o $@

$(OBJS): xnms.h Makefile

