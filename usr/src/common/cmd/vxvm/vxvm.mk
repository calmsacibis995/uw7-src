# @(#)cmd.vxvm:vxvm.mk	1.5 3/3/97 03:36:03 - cmd.vxvm:vxvm.mk
#ident	"@(#)cmd.vxvm:vxvm.mk	1.5"

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

OS = unixware

all:
	@echo "Making all in cmd/vxvm"
	cd $(OS); \
	$(MAKE) -f $(OS).mk CMD_MAKE="$(CMD_MAKE)" \
		DEV1="$(DEV1)" all

lint:
	@echo "Making lint in cmd/vxvm"
	cd $(OS); \
	$(MAKE) -f $(OS).mk CMD_MAKE="$(CMD_MAKE)" \
		DEV1="$(DEV1)"  lint

install:
	@echo "Making install in cmd/vxvm"
	cd $(OS); \
	$(MAKE) -f $(OS).mk CMD_MAKE="$(CMD_MAKE)" \
		DEV1="$(DEV1)" install

headinstall:
	@echo "Making headinstall in cmd/vxvm"
	cd $(OS); \
	$(MAKE) -f $(OS).mk CMD_MAKE="$(CMD_MAKE)" \
		DEV1="$(DEV1)" headinstall

clean:
	@echo "Making clean in cmd/vxvm"
	cd $(OS); \
	$(MAKE) -f $(OS).mk CMD_MAKE="$(CMD_MAKE)" \
		DEV1="$(DEV1)" clean
lintclean:
	@echo "Making lintclean in cmd/vxvm"
	cd $(OS); \
	$(MAKE) -f $(OS).mk lintclean

clobber:
	@echo "Making clobber in $(OS)"
	cd $(OS); \
	$(MAKE) -f $(OS).mk CMD_MAKE="$(CMD_MAKE)" \
		DEV1="$(DEV1)" clobber
