#/* @(#)src/cmd/vxvm/common/support/support.mk	1.1 10/16/96 02:21:33 -  */
#ident	"@(#)cmd.vxvm:common/support/support.mk	1.2"

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

COM_BINTARGETS = egettxt \
	  vxcheckda \
	  vxslicer \
          vxspare \
	  vxbootsetup \
	  vxcntrllist \
	  vxdevlist \
	  vxdisksetup \
	  vxeeprom \
	  vxencap \
	  vxmkboot \
	  vxmksdpart \
	  vxpartadd \
	  vxpartinfo \
	  vxpartrm \
	  vxroot \
	  vxrootmir \
	  vxswapreloc \
	  vxunroot \
	  strtovoff \
	  vxckdiskrm \
	  vxdiskunsetup \
	  $(COMMON_DIR)/vxcap-part \
	  $(COMMON_DIR)/vxcap-vol \
	  $(COMMON_DIR)/vxdiskrm \
	  $(COMMON_DIR)/vxevac \
	  $(COMMON_DIR)/vxmirror \
	  $(COMMON_DIR)/vxnewdmname \
	  $(COMMON_DIR)/vxpartrmall \
	  $(COMMON_DIR)/vxreattach \
	  $(COMMON_DIR)/vxrelocd \
	  $(COMMON_DIR)/vxresize \
	  $(COMMON_DIR)/vxsparecheck \
	  $(COMMON_DIR)/vxtaginfo

COM_ETCSBIN_TARGETS = vxparms

COM_LIBTARGETS = vxcommon \
	vxroot.files

COM_DIAGTARGETS = $(COMMON_DIR)/vxstartup \
	vxsetup

COM_VCKDSKRM_OBJS = $(COMMON_DIR)/vxckdiskrm.o
COM_VCKDSKRM_LINT_OBJS = vxckdiskrm.ln
COM_VCKDSKRM_SRCS = $(COMMON_DIR)/vxckdiskrm.c

COM_STRTOVOFF_OBJS = $(COMMON_DIR)/strtovoff.o
COM_STRTOVOFF_LINT_OBJS = strtovoff.ln
COM_STRTOVOFF_SRCS = $(COMMON_DIR)/strtovoff.c

COM_OBJS = $(COM_EGETTXT_OBJS) \
	$(COM_VCHKDA_OBJS) \
	$(COM_VCKDSKRM_OBJS) \
	$(COM_VPARMS_OBJS) \
	$(COM_VPARMS_OBJS) \
	$(COM_VSLICER_OBJS) \
	$(COM_VSPARE_OBJS) \
	$(COM_STRTOVOFF_OBJS)

COM_LINT_OBJS = $(COM_EGETTXT_LINT_OBJS) \
	$(COM_VCHKDA_LINT_OBJS) \
	$(COM_VCKDSKRM_LINT_OBJS) \
	$(COM_VPARMS_LINT_OBJS) \
	$(COM_VPARMS_LINT_OBJS) \
	$(COM_VSLICER_LINT_OBJS) \
	$(COM_VSPARE_LINT_OBJS) \
	$(COM_STRTOVOFF_LINT_OBJS)

COM_LOCALINC = -I../../common/libcmd
