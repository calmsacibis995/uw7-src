#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)/usr/src/ihvkit/pdi/ictha/ictha.mk.sl 1.1 UW2.0 12/05/94 64562 NOVELL"

.SUFFIXES: .ln
.c.ln:
	echo "\n$(DIR)/$*.c:" > $*.L
	-$(LINT) $(LINTFLAGS) $(LOCALDEF) $(INCLIST) -c -u $*.c >> $*.L

MAKEFILE=	ictha.mk
DIR = io/hba/ictha
INC = /usr/include
INCLIST = -I$(INC) -I$(INC)/sys
INS = install
OWN = bin
GRP = bin
HINSPERM = -m 644 -u $(OWN) -g $(GRP)

LOCALDEF = -D_KERNEL -DSTATIC=static

ETC = /etc
CONF = $(ETC)/conf
CONFBIN = $(CONF)/bin
IDINSTALL = $(CONFBIN)/idinstall
IDBUILD = $(CONFBIN)/idbuild
ICTHA = ictha.cf/Driver.o

DRIVER = ictha
IHVKIT_BASE = /usr/src/ihvkit/pdi
DRIVER_BASE = $(IHVKIT_BASE)/$(DRIVER)
DRIVER_CFG_BASE = $(DRIVER_BASE)/$(DRIVER).cf
HBA_FLP_BASE = $(DRIVER_BASE)/$(DRIVER).hbafloppy
HBA_FLP_BASE_TMP = $(HBA_FLP_BASE)/$(DRIVER)/tmp
HBA_FLP_OBJECT_LOC = $(HBA_FLP_BASE)/$(DRIVER)/tmp/$(DRIVER)

OBJECTS = Space.c System Master Drvmap disk.cfg loadmods ictha ictha.h


LINT = /usr/bin/lint
LINTFLAGS = -k -n -s
LINTDIR = ./lintdir
LFILE = $(LINTDIR)/ictha.ln
LFILES = ictha.ln


FILES = ictha.o
CFILES = ictha.c

SRCFILES = $(CFILES)

.c.o:
	$(CC) $(CFLAGS) $(LOCALDEF) $(INCLIST) -c $<

all:	$(ICTHA)

install:	headinstall all
		(cd ictha.cf ; \
		$(IDINSTALL) -d $(DRIVER) > /dev/null 2>&1; \
		$(IDINSTALL)  -a -k $(DRIVER); \
		$(IDBUILD) -M  $(DRIVER); \
		cp $(CONF)/mod.d/$(DRIVER) .; \
		grep $(DRIVER) $(ETC)/mod_register | sort | uniq >loadmods; \
		cp disk.cfg $(CONF)/pack.d/$(DRIVER) )

$(ICTHA):	$(FILES)
		$(LD) -r -o $(ICTHA) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(ICTHA)
	rm -rf $(HBA_FLP_BASE_TMP)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e $(DRIVER) 

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
	ictha.h \
	hba.h

headinstall: $(sysHeaders)
	cp ictha.h ictha.cf
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f /usr/include/sys $(HINSPERM) $$f; \
	 done

hbafloppy:
	( cd $(DRIVER_BASE); \
	rm -rf $(HBA_FLP_OBJECT_LOC); \
	mkdir -p $(HBA_FLP_OBJECT_LOC); \
	cd $(DRIVER_CFG_BASE); \
	for i in ${OBJECTS}; do \
		cp $$i $(HBA_FLP_OBJECT_LOC); \
	done ; \
	cd $(HBA_FLP_BASE); \
	./bldscript; \
	)
