#	Copyright (c) 1993 UNIX System Laboratories, Inc.
#	(a wholly-owned subsidiary of Novell, Inc.).
#	All Rights Reserved.

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF UNIX SYSTEM
#	LABORATORIES, INC. (A WHOLLY-OWNED SUBSIDIARY OF NOVELL, INC.).
#	The copyright notice above does not evidence any actual or
#	intended publication of such source code.

#	Copyright (c) 1992-1994  Intel Corporation
#	All Rights Reserved

#	INTEL CORPORATION CONFIDENTIAL INFORMATION

#	This software is supplied to USL under the terms of a license
#	agreement with Intel Corporation and may not be copied nor
#	disclosed except in accordance with the terms of that agreement.

#ident	"@(#)kern-i386:io/mtrr/mtrr.mk	1.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	mtrr.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
P6STEPLET=A
P6STEPNUM=1
P6STEPPING=$(P6STEPLET)$(P6STEPNUM)
#LOCALDEF = -DKDEBUG -UDEBUG
LOCALDEF =  -UDEBUG  -DNODEBUG -DP6$(P6STEPPING) -DP6$(P6STEPLET) -DP6
DIR = io/mtrr

MTRRD = mtrr.cf/Driver.o
LFILE = $(LINTDIR)/mtrr.ln

FILES = driver.o 

CFILES = driver.c 

LFILES = mtrr.ln

all: $(MTRRD) $(STEPMTRRD)

install: all
	(cd mtrr.cf; $(IDINSTALL) -R$(CONF) -M mtrr)

$(MTRRD): $(FILES) $(MAKEFILE)
	$(LD) -r -o $(MTRRD) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(MTRRD) $(STEPMTRRD)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e mtrr

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
	mtrr.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

driver.o:	$(MAKEFILE)

include $(UTSDEPEND)

include $(MAKEFILE).dep
