#ident	"@(#)kern-i386at:io/hba/efp2/efp2.mk	1.2"
#ident	"$Header$"

#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


include $(UTSRULES)

MAKEFILE=	efp2.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/efp2

EFP2 = efp2.cf/Driver.o
LFILE = $(LINTDIR)/efp2.ln

FILES = efp2.o \
	scsi_eisa.o \
	ior0005.o

CFILES = efp2.c \
	scsi_eisa.c \
	ior0005.c

LFILES = efp2.ln scsi_eisa.ln ior0005.ln

SRCFILES = $(CFILES)

all:	$(EFP2)

install:	all
		(cd efp2.cf ; $(IDINSTALL) -R$(CONF) -M efp2; \
		rm -f $(CONF)/pack.d/efp2/disk.cfg;	\
		cp disk.cfg $(CONF)/pack.d/efp2	)

$(EFP2):	$(FILES)
		$(LD) -r -o $(EFP2) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(EFP2)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e efp2

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
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

sysHeaders = \
	efp2.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
