# @(#)src/cmd/vxvm/common/raid5/raid5.mk	1.1 10/16/96 02:21:14 - 
#ident	"@(#)cmd.vxvm:common/raid5/raid5.mk	1.1"

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

COMMON_GEN_DIR = ../../common/gen

COM_TARGETS = vxmake \
	vxmend \
	vxplex \
	vxsd \
	vxvol

COM_VMAKE_OBJS = $(COMMON_DIR)/volmake.o \
	$(COMMON_GEN_DIR)/genextern.o
COM_VMAKE_LINT_OBJS = volmake.ln \
	genextern.ln
COM_VMAKE_SRCS = volmake.c \
	$(COMMON_GEN_DIR)/genextern.c

COM_VMEND_OBJS = $(COMMON_DIR)/volmend.o \
	$(COMMON_GEN_DIR)/genextern.o \
	$(COMMON_GEN_DIR)/gencommon.o
COM_VMEND_LINT_OBJS = volmend.ln \
	genextern.ln \
	gencommon.ln
COM_VMEND_SRCS = $(COMMON_DIR)/volmend.c \
	$(COMMON_GEN_DIR)/genextern.c \
	$(COMMON_GEN_DIR)/gencommon.c

COM_VPLEX_OBJS = $(COMMON_DIR)/volplex.o \
	$(COMMON_DIR)/raid5plxopts.o \
	$(COMMON_DIR)/raid5extern.o \
	$(COMMON_DIR)/raid5common.o \
	$(COMMON_DIR)/comraid5.o \
	$(COMMON_GEN_DIR)/gencommon.o \
	$(COMMON_DIR)/r5logs.o \
	$(COMMON_DIR)/r5io.o
COM_VPLEX_LINT_OBJS = volplex.ln \
	raid5plxopts.ln \
	raid5extern.ln \
	raid5common.ln \
	comraid5.ln \
	gencommon.ln \
	r5logs.ln \
	r5io.ln
COM_VPLEX_SRCS = $(COMMON_DIR)/volplex.c \
	$(COMMON_DIR)/raid5plxopts.c \
	$(COMMON_DIR)/raid5extern.c \
	$(COMMON_DIR)/raid5common.c \
	$(COMMON_DIR)/comraid5.c \
	$(COMMON_GEN_DIR)/gencommon.c \
	$(COMMON_DIR)/r5logs.c \
	$(COMMON_DIR)/r5io.c

COM_VSD_OBJS = $(COMMON_DIR)/volsd.o \
	$(COMMON_DIR)/raid5extern.o \
	$(COMMON_GEN_DIR)/gencommon.o \
	$(COMMON_DIR)/comraid5.o \
	$(COMMON_DIR)/raid5common.o \
	$(COMMON_DIR)/r5io.o

COM_VSD_LINT_OBJS = volsd.ln \
	raid5extern.ln \
	gencommon.ln \
	comraid5.ln \
	raid5common.ln \
	r5io.ln

COM_VSD_SRCS = $(COMMON_DIR)/volsd.c \
	$(COMMON_DIR)/raid5extern.c \
	$(COMMON_GEN_DIR)/gencommon.c \
	$(COMMON_DIR)/comraid5.c \
	$(COMMON_DIR)/raid5common.c \
	$(COMMON_DIR)/r5io.c

COM_VVOL_OBJS = $(COMMON_DIR)/volume.o \
	$(COMMON_DIR)/raidvolopts.o \
	$(COMMON_GEN_DIR)/gencommon.o \
	$(COMMON_GEN_DIR)/genlog.o \
	$(COMMON_DIR)/raid5extern.o \
	$(COMMON_DIR)/raid5common.o \
	$(COMMON_DIR)/r5io.o \
	$(COMMON_DIR)/r5logs.o \
	$(COMMON_DIR)/r5start.o \
	$(COMMON_GEN_DIR)/comklog.o \
	$(COMMON_DIR)/comraid5.o

COM_VVOL_LINT_OBJS = volume.ln \
	raidvolopts.ln \
	gencommon.ln \
	genlog.ln \
	raid5extern.ln \
	raid5common.ln \
	r5io.ln \
	r5logs.ln \
	r5start.ln \
	comklog.ln \
	comraid5.ln

COM_VVOL_SRCS = $(COMMON_DIR)/volume.c \
	$(COMMON_DIR)/raidvolopts.c \
	$(COMMON_GEN_DIR)/gencommon.c \
	$(COMMON_GEN_DIR)/genlog.c \
	$(COMMON_DIR)/raid5extern.c \
	$(COMMON_DIR)/raid5common.c \
	$(COMMON_DIR)/r5io.c \
	$(COMMON_DIR)/r5logs.c \
	$(COMMON_DIR)/r5start.c \
	$(COMMON_GEN_DIR)/comklog.c \
	$(COMMON_DIR)/comraid5.c

COM_LOCALINC = -I../../common/libcmd \
	-I$(COMMON_DIR) \
	-I$(COMMON_GEN_DIR)

COM_LOCALDEF = -DUSE_TYPE='"raid5"'

COM_OBJS = $(COMMON_DIR)/volmake.o \
	$(COMMON_DIR)/volmend.o \
	$(COMMON_DIR)/volume.o \
	$(COMMON_DIR)/raidvolopts.o \
	$(COMMON_DIR)/raid5extern.o \
	$(COMMON_DIR)/raid5common.o \
	$(COMMON_DIR)/r5io.o \
	$(COMMON_DIR)/r5logs.o \
	$(COMMON_DIR)/r5start.o \
	$(COMMON_DIR)/comraid5.o \
	$(COMMON_DIR)/volplex.o \
	$(COMMON_DIR)/raid5plxopts.o \
	$(COMMON_DIR)/volsd.o \
	$(COMMON_GEN_DIR)/genextern.o \
	$(COMMON_GEN_DIR)/gencommon.o \
	$(COMMON_GEN_DIR)/genlog.o \
	$(COMMON_GEN_DIR)/comklog.o

COM_LINT_OBJS = volmake.ln \
	volmend.ln \
	volume.ln \
	raidvolopts.ln \
	raid5extern.ln \
	raid5common.ln \
	r5io.ln \
	r5logs.ln \
	r5start.ln \
	comraid5.ln \
	volplex.ln \
	raid5plxopts.ln \
	volsd.ln \
	genextern.ln \
	gencommon.ln \
	genlog.ln \
	comklog.ln

COM_SRCS = $(COMMON_DIR)/volmake.c \
	$(COMMON_DIR)/volmend.c \
	$(COMMON_DIR)/volume.c \
	$(COMMON_DIR)/raidvolopts.c \
	$(COMMON_DIR)/raid5extern.c \
	$(COMMON_DIR)/raid5common.c \
	$(COMMON_DIR)/r5io.c \
	$(COMMON_DIR)/r5logs.c \
	$(COMMON_DIR)/r5start.c \
	$(COMMON_DIR)/comraid5.c \
	$(COMMON_DIR)/volplex.c \
	$(COMMON_DIR)/raid5plxopts.c \
	$(COMMON_DIR)/volsd.c \
	$(COMMON_GEN_DIR)/genextern.c \
	$(COMMON_GEN_DIR)/gencommon.c \
	$(COMMON_GEN_DIR)/genlog.c

COM_HDRS = $(COMMON_DIR)/volraid5.h
