# @(#)src/uw/kernel/vxportal/vxportal.mk	3.8.9.1 12/01/97 20:03:09 - 
#ident	"@(#)vxfs:src/uw/kernel/vxportal/vxportal.mk	3.8.9.1"
#ident	"@(#)kern-i386:io/vxportal/vxportal.mk	1.1.3.1"
#
# Copyright (c) 1997 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
# UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
# LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
# IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
# OR DISCLOSURE.
#
# THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
# TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
# OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
# EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
#
#		RESTRICTED RIGHTS LEGEND
# USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
# SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
# (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
# COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
#		VERITAS SOFTWARE
# 1600 PLYMOUTH STREET, MOUNTAIN VIEW, CA 94043
#

include $(UTSRULES)

KBASE	= ../..
VXBASE	 = ../..
#TED	 = -DTED_
TED	 =
LOCALDEF = -D_FSKI=2 $(TED)
LOCALINC = -I $(VXBASE) -I $(VXBASE)/fs/vxfs

VXPORTAL = vxportal.cf/Driver.o
MODULES = $(VXPORTAL)

LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/vxportal.ln

PROBEFILE = vx_portal.c
MAKEFILE = vxportal.mk
BINARIES = $(VXPORTAL)

CFILES = \
	vx_portal.c

LFILES = \
	vx_portal.ln

VXPORTAL_OBJS = \
	vx_portal.o

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
		for fl in $(BINARIES); do \
			if [ ! -r $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi

clean:
	-rm -f $(VXPORTAL_OBJS) $(LFILES) *.L

clobber: clean
	-$(IDINSTALL) -e -R$(CONF) -d vxportal
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

$(LINTDIR):
	[ -d $@ ] || mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(CFILES);	\
	do \
		echo $$i; \
	done

#
# Configuration Section
#

install: all
	(cd vxportal.cf; $(IDINSTALL) -R$(CONF) -M vxportal)

headinstall:

binaries: $(BINARIES)

$(BINARIES): $(VXPORTAL_OBJS)
	$(LD) -r -o $@ $(VXPORTAL_OBJS)

include $(UTSDEPEND)

include $(MAKEFILE).dep
