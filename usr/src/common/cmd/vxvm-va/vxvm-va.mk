# @(#)src/cmd/vxvm/vxvm.mk
#ident	"@(#)cmd.vxva:vxvm-va/vxvm-va.mk	1.2"

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

VXVATOP = $(ROOT)/$(MACH)/opt/vxvm-va

all:
	@echo "Making all in cmd/vxva"
	cd src; \
	$(MAKE) -f Makefile all

lint:
	@echo "Making lint in cmd/vxva"
	cd src; \
	$(MAKE) -f Makefile lint

install:
	@echo  "Making install in cmd/vxva"
	cd src; \
	$(MAKE) -f Makefile install VXVATOP="$(VXVATOP)"

headinstall:
	@echo "Making headinstall in cmd/vxva"

clean:
	@echo "Making clean in cmd/vxva"
	cd src; \
	$(MAKE) -f Makefile clean

lintclean: clean
	@echo "Making lintclean in cmd/vxva"

clobber:
	@echo "Making clobber in cmd/vxva"
	cd src; \
	$(MAKE) -f Makefile clobber

