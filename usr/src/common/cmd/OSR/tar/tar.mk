#ident	"@(#)OSRcmds:tar/tar.mk	1.2"
#	@(#) tar.mk 26.1 95/04/26 
#
#	Copyright (C) The Santa Cruz Operation, 1986-1995.
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, Microsoft Corporation
#	and AT&T, and should be treated as Confidential.
#
include $(CMDRULES)
include ../make.inc

INTL=-DINTL

# The data checksumming code is presently considered expeirmental
# and not to be shipped as part of XENIX.  -sco!blf, 3 June 1987.
#	DEFINES	= -DCHKSUM
DEFINES	=

CFLAGS	= -O $(DEFINES)

DBIN	= $(OSRDIR)
# DETC	= ${ROOT}/etc/default

all:	tar

install:	all
	# cp def135.src $(DETC)/tar135
	# cp def96.src $(DETC)/tar96
	# cp def135.src $(DETC)/tar	<- Should be merged with the
	#					current UW /etc/default/tar
	$(DOCATS) -d NLS $@
	$(INS) -f $(DBIN) tar

cmp:	all
	cmp tar $(DBIN)/tar
	# cmp def135.src $(DETC)/tar135
	# cmp def96.src $(DETC)/tar96
	# cmp def135.src $(DETC)/tar

clean:
	rm -f tar.o tar_msg.h

clobber:	clean
	rm -f tar
	$(DOCATS) -d NLS $@

lintall:
	lint -abchx tar.c

tar:	tar.o
	$(CC) $(CFLAGS) -o tar tar.o -lcmd $(LDFLAGS) ../lib/libos/errorl.o \
		../lib/libos/errorv.o

tar.o: tar.c tar_msg.h ../include/osr.h

tar_msg.h :	NLS/en/tar.gen
	$(MKCATDEFS) tar $? > /dev/null
	cat tar_msg.h | sed 's/"tar.cat@Unix"/"OSRtar.cat@Unix"/g' > tar_msgt.h
	mv tar_msgt.h tar_msg.h
