# @(#)cmd.vxvm:unixware/locale/locale.mk	1.5 10/10/97 15:58:56 - cmd.vxvm:unixware/locale/locale.mk
#ident	"@(#)cmd.vxvm:unixware/locale/locale.mk	1.5"

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

include $(CMDRULES)


VXVMLOCALE=./C/LC_MESSAGES
SYSLOCALE=$(ROOT)/$(MACH)/usr/lib/locale/$(VXVMLOCALE)

# Pick a probe file as the key of restricted source
PROBEFILE = $(VXVMLOCALE)/vxvmmsg.str
MAKEFILE = locale.mk

SRC = \
	$(VXVMLOCALE)/vxvmmsg.str \
	$(VXVMLOCALE)/libvxvm.str \
	$(VXVMLOCALE)/vxvmshm.str

OBJ = \
	$(VXVMLOCALE)/vxvm.mesg \
	$(VXVMLOCALE)/libvxvm \
	$(VXVMLOCALE)/vxvmshm

.IGNORE:

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) cataloge $(MAKEARGS) ;\
	else \
		for f in $(OBJ); do \
			if [ ! -f $$f ]; then \
				echo "ERROR: $$f is missing" 1>&2 ;\
				false ;\
				break; \
			fi \
		done \
	fi

cataloge:
	mkmsgs -o $(VXVMLOCALE)/vxvmmsg.str $(VXVMLOCALE)/vxvm.mesg
	mkmsgs -o $(VXVMLOCALE)/libvxvm.str $(VXVMLOCALE)/libvxvm
	mkmsgs -o $(VXVMLOCALE)/vxvmshm.str $(VXVMLOCALE)/vxvmshm

install: all
	[ -d $(SYSLOCALE) ] || mkdir -p $(SYSLOCALE)
	if [ -f $(PROBEFILE) ]; then \
		$(INS) -f $(SYSLOCALE) -m 0644 -u root -g sys C/LC_MESSAGES/vxvmmsg.str ; \
		$(INS) -f $(SYSLOCALE) -m 0644 -u root -g sys C/LC_MESSAGES/libvxvm.str ; \
		$(INS) -f $(SYSLOCALE) -m 0644 -u root -g sys C/LC_MESSAGES/vxvmshm.str ; \
		$(INS) -f $(SYSLOCALE) -m 0644 -u root -g sys C/LC_MESSAGES/vxvm.mesg ; \
		$(INS) -f $(SYSLOCALE) -m 0644 -u root -g sys C/LC_MESSAGES/libvxvm ; \
		$(INS) -f $(SYSLOCALE) -m 0644 -u root -g sys C/LC_MESSAGES/vxvmshm ; \
	else\
		$(INS) -f $(SYSLOCALE) -m 0644 -u root -g sys C/LC_MESSAGES/vxvm.mesg ; \
		$(INS) -f $(SYSLOCALE) -m 0644 -u root -g sys C/LC_MESSAGES/libvxvm ; \
		$(INS) -f $(SYSLOCALE) -m 0644 -u root -g sys C/LC_MESSAGES/vxvmshm ; \
	fi


headinstall:

clean:
	@if [ -f $(PROBEFILE) ]; then \
		rm -f $(OBJ) ; \
	fi

clobber: clean

