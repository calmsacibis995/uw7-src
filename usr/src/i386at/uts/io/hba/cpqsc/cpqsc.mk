#ident	"@(#)kern-i386at:io/hba/cpqsc/cpqsc.mk	1.10.4.5"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

include $(UTSRULES)

MAKEFILE= cpqsc.mk
KBASE = ../../..

DRVNAME = cpqsc
CPQSC = cpqsc.cf/Driver.o
PROBEFILE = cpqsc.c
BINARIES = $(CPQSC)

DRVFILES = \
	Master \
	System \
	Space.c \
	disk.cfg \

INSPERM = -m 644 -u root -g sys
HINSPERM = -m 644 -u bin -g bin

all: $(BINARIES)

install: all
	(cd $(DRVNAME).cf; \
	$(IDINSTALL) -R$(CONF) -M $(DRVNAME); \
	rm -f $(CONF)/pack.d/$(DRVNAME)/disk.cfg; \
	$(INS) -f $(CONF)/pack.d/$(DRVNAME) $(INSPERM) disk.cfg )

clean:

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e $(DRVNAME)

sysHeaders = cpqsc.h

headinstall:	$(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for i in $(sysHeaders); \
	do \
		$(INS) -f $(INC)/sys $(HINSPERM) $$i; \
	done

FRC: 

include $(UTSDEPEND)

include $(MAKEFILE).dep

