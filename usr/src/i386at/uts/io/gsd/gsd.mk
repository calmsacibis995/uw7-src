#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)gsd.mk	1.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	gsd.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/gsd

GSD = gsd.cf/Driver.o
LFILE = $(LINTDIR)/kd.ln

FILES = \
	gsddrv.o \
	gsdgraph.o \
	gsdtcl.o \
	gsdwrap.o

CFILES = \
	gsddrv.c \
	gsdgraph.c \
	gsdtcl.c \
	gsdwrap.c

LFILES = \
	gsddrv.ln \
	gsdgraph.ln \
	gsdtcl.ln \
	gsdwrap.ln


all: $(GSD)

gsddrv.o: gsd.h gsddrv.c
gsdgraph.o: gsd.h gsdgraph.c
gsdtcl.o: gsd.h gsdtcl.c
gsdwrap.o: gsd.h gsdwrap.c

install: all
	(cd gsd.cf; $(IDINSTALL) -R$(CONF) -M gsd)

$(GSD): $(FILES)
	$(LD) -r -o $(GSD) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(GSD)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e gsd 

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(CFILES); do \
		echo $$i; \
	done

sysHeaders = \
	gsd.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
