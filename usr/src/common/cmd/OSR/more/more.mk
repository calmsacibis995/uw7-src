#ident	"@(#)OSRcmds:more/more.mk	1.2"
#	@(#) more.mk 57.1 96/08/08 
#
#	Copyright (C) The Santa Cruz Operation, 1994-1996.
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#
#
#  MODIFICATION HISTORY
#
#	30 Mar 1994	scol!anthonys
#	- Rewritten from scratch
#	19 Apr 1994	scol!ianw
#	- Changed to install in usr/bin (instead of bin).
#	17 Feb 95	scol!donaldp
#	- Added support for multiple message catalogues.
#	29 Aug 1995	scol!ashleyb
#	- Added support for localised help files.
#

include $(CMDRULES)
include ../make.inc

INTL	= -DINTL
INSDIR	= $(OSRDIR)
LIBDIR  = $(ROOT)/$(MACH)/usr/lib
# CFLAGS	= -O $(XPGUTIL)
CFLAGS	= -O
LDLIBS	= -lcurses -lcmd
LFLAGS	= $(LDLIBS)

OBJS=   ch.o command.o decode.o help.o input.o line.o linenum.o main.o \
	option.o os.o output.o position.o prim.o screen.o signal.o tags.o \
	ttyin.o

NLS_FR	= $(ROOT)/$(MACH)/usr/lib/nls/misc/fr_FR.ISO8859-1/Unix
NLS_DE	= $(ROOT)/$(MACH)/usr/lib/nls/misc/de_DE.ISO8859-1/Unix

all:	more

more:	../include/osr.h more_msg.h $(OBJS) \
	$(SYSINC)/types.h
	$(CC) -I. $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LFLAGS)

more_msg.h:	NLS/en/more.gen
	$(MKCATDEFS) more $? >/dev/null
	cat more_msg.h | sed 's/"more.cat@Unix"/"OSRmore.cat@Unix"/g' > more_msgt.h
	mv more_msgt.h more_msg.h

install: all
	cp NLS/en/more.help $(LIBDIR)/OSRmore.help
	[ -d $(NLS_FR) ] || mkdir -p $(NLS_FR)
	cp NLS/fr/more.help $(NLS_FR)/OSRmore.help
	[ -d $(NLS_DE) ] || mkdir -p $(NLS_DE)
	cp NLS/de/more.help $(NLS_DE)/OSRmore.help
	$(DOCATS) -d NLS $@
	$(INS) -f $(INSDIR) more

clean:
	rm -f *.o more_msg.h

clobber: clean
	rm -f more
	$(DOCATS) -d NLS $@
