#ident	"@(#)ppputils.mk	1.2"

#
# Copyrighted as an unpublished work.
# (c) Copyright 1997 SCO Ltd
# All rights reserved.
#

#
# This makefile builds pppattach, pppdetach and pppstat 
# It requires libraries (libppprt.so)
#

include $(CMDRULES)

LDLIBS = -lsocket -lgen  -lx -L .. -lnsl -lpppcmd -lcurses
LOCALDEF = -DSYSV -DSVR4 -DSTATIC=
LOCALINC=-I$(ROOT)/$(MACH)/usr/include -I../include

OWN=		bin
GRP=		bin

PPP_MSG = ppp_msg.h
PPP_GEN = NLS/english/ppp.gen

PPP_CFILES = pppattach.c
PPP_FILES  = pppattach.o
PPPATTACH  = pppattach
PPPDETACH  = $(USRBIN)/pppdetach
PPPSTATUS  = $(USRBIN)/pppstatus
PPPLINKADD = $(USRBIN)/ppplinkadd
PPPLINKDROP  = $(USRBIN)/ppplinkdrop

SRCFILES = $(PPP_CFILES)
FILES = $(PPP_FILES)
TARGS = $(PPPATTACH)

all:	$(TARGS)

clean:
	-rm -f $(FILES) $(PPP_MSG)
	$(DOCATS) -d NLS $@

clobber: clean
	-rm -f $(TARGS)
	$(DOCATS) -d NLS $@

config:

headinstall:

install:	all
	[ -d $(USRBIN) ] || mkdir -p $(USRBIN)
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) $(PPPATTACH)
	rm -f $(PPPDETACH) $(PPPSTATUS) $(PPPLINKADD) $(PPPLINKDROP)
	-ln $(USRBIN)/$(PPPATTACH) $(PPPDETACH)
	-ln $(USRBIN)/$(PPPATTACH) $(PPPSTATUS)
	-ln $(USRBIN)/$(PPPATTACH) $(PPPLINKADD)
	-ln $(USRBIN)/$(PPPATTACH) $(PPPLINKDROP)
	$(DOCATS) -d NLS $@

fnames:
	@for f in $(SRCFILES); do \
		echo $$f; \
	done

lintit:

$(PPP_MSG):	$(PPP_GEN)
		$(MKCATDEFS) ppp $(PPP_GEN) > /dev/null

$(PPPATTACH):	$(PPP_MSG) $(PPP_FILES)
		$(CC) -g -o $(PPPATTACH) $(PPP_FILES) \
			$(LDLIBS) $(SHLIBS) -L ../ppptalk -lpppparse

#		$(CC) -g -o $(PPPATTACH) $(LDFLAGS) $(PPP_FILES) \
#
# Header dependencies
#
