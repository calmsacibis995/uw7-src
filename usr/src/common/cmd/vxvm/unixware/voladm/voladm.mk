# @(#)cmd.vxvm:unixware/voladm/voladm.mk	1.3 3/3/97 03:33:34 - cmd.vxvm:unixware/voladm/voladm.mk
#ident	"@(#)cmd.vxvm:unixware/voladm/voladm.mk	1.3"

# Copyright(C)1996 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
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
#               RESTRICTED RIGHTS LEGEND
# USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
# SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
# (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
# COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
#               VERITAS SOFTWARE
# 1600 PLYMOUTH STREET, MOUNTAIN VIEW, CA 94043

COMMON_DIR = ../../common/voladm
LINKDIR    = ../../unixware/voladm

include $(COMMON_DIR)/voladm.mk

include $(CMDRULES)

# Force cmdrules include from $(TOOLS) to $(ROOT)/$(MACH)
INC = $(ROOT)/$(MACH)/usr/include

VXDIR	= $(ROOT)/$(MACH)/usr/lib/vxvm
ADMDIR	= $(VXDIR)/voladm.d
ADMBIN	= $(ADMDIR)/bin
ADMLIB	= $(ADMDIR)/lib
ADMHELP	= $(ADMDIR)/help

USRSBINTARGETS = $(COM_USRSBINTARGETS)

BINTARGETS = $(COM_BINTARGETS)

LIBTARGETS = $(COM_LIBTARGETS)

HELPFILES = $(COM_HELPFILES)

VOLADM_TARGETS = $(USRSBINTARGETS) \
	$(BINTARGETS) \
	$(LIBTARGETS)

all: $(VOLADM_TARGETS)

install: install_system_sbin install_bin install_lib install_help

lint:  
	@echo "Nothing to lint voladm"

lintclean:
	@echo "Nothing to lintclean voladm"

install_dirs:
	[ -d $(VXDIR) ] || mkdir $(VXDIR)
	[ -d $(ADMDIR) ] || mkdir $(ADMDIR)
	[ -d $(ADMBIN) ] || mkdir $(ADMBIN)
	[ -d $(ADMLIB) ] || mkdir $(ADMLIB)
	[ -d $(ADMHELP) ] || mkdir $(ADMHELP)

install_system_sbin: all install_dirs
	for f in $(USRSBINTARGETS) ; \
	do \
		rm -f $(USRSBIN)/$$f ; \
		$(INS) -f $(USRSBIN) -m 0555 -u root -g sys $$f ; \
	done

install_bin: all install_dirs
	for f in $(BINTARGETS) ; \
	do \
		rm -f $(ADMBIN)/$$f; \
		$(INS) -f $(ADMBIN) -m 0555 -u root -g sys $$f ; \
	done

install_lib: all install_dirs $(LIBTARGETS)
	for f in $(LIBTARGETS) ; \
	do \
		rm -f $(ADMLIB)/$$f ; \
		$(INS) -f $(ADMLIB) -m 0644 -u root -g sys $$f ; \
	done

install_help: all install_dirs $(HELPFILES)
	for f in $(HELPFILES) ; \
	do \
		rm -f $(ADMHELP)/$$f; \
		$(INS) -f $(ADMHELP) -m 0644 -u root -g sys $$f ; \
	done

headinstall:

clean:

clobber: clean
	rm -f $(USRSBINTARGETS) $(BINTARGETS)
	for f in $(USRSBINTARGETS) $(BINTARGETS) ; \
	do \
		rm -f `basename $$f` ; \
	done
