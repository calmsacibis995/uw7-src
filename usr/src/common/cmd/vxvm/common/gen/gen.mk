# @(#)src/cmd/vxvm/common/gen/gen.mk	1.1 10/16/96 02:20:12 - 
#ident	"@(#)cmd.vxvm:common/gen/gen.mk	1.1"

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

COM_TARGETS = vxinfo \
	vxmake \
	vxmend \
	vxplex \
	vxsd \
	vxvol

COM_VINFO_OBJS = $(COMMON_DIR)/volinfo.o \
	$(COMMON_DIR)/genextern.o \
	$(COMMON_DIR)/gencommon.o
COM_VINFO_LINT_OBJS = volinfo.ln \
	genextern.ln \
	gencommon.ln
COM_VINFO_SRCS = $(COMMON_DIR)/volinfo.c \
	$(COMMON_DIR)/genextern.c \
	$(COMMON_DIR)/gencommon.c

COM_VMAKE_OBJS = $(COMMON_DIR)/volmake.o \
	$(COMMON_DIR)/genextern.o
COM_VMAKE_LINT_OBJS = volmake.ln \
	genextern.ln
COM_VMAKE_SRCS = $(COMMON_DIR)/volmake.c \
	$(COMMON_DIR)/genextern.c

COM_VMEND_OBJS = $(COMMON_DIR)/volmend.o \
	$(COMMON_DIR)/genextern.o \
	$(COMMON_DIR)/gencommon.o
COM_VMEND_LINT_OBJS = volmend.ln \
	genextern.ln \
	gencommon.ln
COM_VMEND_SRCS = $(COMMON_DIR)/volmend.c \
	$(COMMON_DIR)/genextern.c \
	$(COMMON_DIR)/gencommon.c

COM_VPLEX_OBJS = $(COMMON_DIR)/volplex.o \
	$(COMMON_DIR)/genplexdet.o \
	$(COMMON_DIR)/genplxopts.o \
	$(COMMON_DIR)/genextern.o \
	$(COMMON_DIR)/gencommon.o \
	$(COMMON_DIR)/genlog.o
COM_VPLEX_LINT_OBJS = volplex.ln \
	genplexdet.ln \
	genplxopts.ln \
	genextern.ln \
	gencommon.ln \
	genlog.ln
COM_VPLEX_SRCS = $(COMMON_DIR)/volplex.c \
	$(COMMON_DIR)/genplexdet.c \
	$(COMMON_DIR)/genplxopts.c \
	$(COMMON_DIR)/genextern.c \
	$(COMMON_DIR)/gencommon.c \
	$(COMMON_DIR)/genlog.c

COM_VSD_OBJS = $(COMMON_DIR)/volsd.o \
	$(COMMON_DIR)/genextern.o \
	$(COMMON_DIR)/gencommon.o \
	$(COMMON_DIR)/genlog.o
COM_VSD_LINT_OBJS = volsd.ln \
	genextern.ln \
	gencommon.ln \
	genlog.ln
COM_VSD_SRCS = $(COMMON_DIR)/volsd.c \
	$(COMMON_DIR)/genextern.c \
	$(COMMON_DIR)/gencommon.c \
	$(COMMON_DIR)/genlog.c

COM_VVOL_OBJS = $(COMMON_DIR)/volume.o \
	$(COMMON_DIR)/genstart.o \
	$(COMMON_DIR)/genvolopts.o \
	$(COMMON_DIR)/genlog.o \
	$(COMMON_DIR)/genextern.o \
	$(COMMON_DIR)/comstart.o \
	$(COMMON_DIR)/gencommon.o \
	$(COMMON_DIR)/comklog.o
COM_VVOL_LINT_OBJS = volume.ln \
	genstart.ln \
	genvolopts.ln \
	genlog.ln \
	genextern.ln \
	comstart.ln \
	gencommon.ln \
	comklog.ln
COM_VVOL_SRCS = $(COMMON_DIR)/volume.c \
	$(COMMON_DIR)/genstart.c \
	$(COMMON_DIR)/genvolopts.c \
	$(COMMON_DIR)/genlog.c \
	$(COMMON_DIR)/genextern.c \
	$(COMMON_DIR)/comstart.c \
	$(COMMON_DIR)/gencommon.c \
	$(COMMON_DIR)/comklog.c

COM_FSGEN_LINKS = vxinfo \
	vxmake \
	vxmend \
	vxsd \
	vxvol

COM_R5_LINKS = vxinfo

COM_LOCALINC = -I../../common/libcmd -I../../common/fsgen

COM_OBJS = $(COMMON_DIR)/volinfo.o \
	$(COMMON_DIR)/volmake.o \
	$(COMMON_DIR)/volmend.o \
	$(COMMON_DIR)/volplex.o \
	$(COMMON_DIR)/volsd.o \
	$(COMMON_DIR)/volume.o \
	$(COMMON_DIR)/genvolopts.o \
	$(COMMON_DIR)/genplxopts.o \
	$(COMMON_DIR)/genstart.o \
	$(COMMON_DIR)/genplexdet.o \
	$(COMMON_DIR)/genlog.o \
	$(COMMON_DIR)/genextern.o \
	$(COMMON_DIR)/gencommon.o \
	$(COMMON_DIR)/comklog.o \
	$(COMMON_DIR)/comstart.o
COM_LINT_OBJS = volinfo.ln \
	volmake.ln \
	volmend.ln \
	volplex.ln \
	volsd.ln \
	volume.ln \
	genvolopts.ln \
	genplxopts.ln \
	genstart.ln \
	genplexdet.ln \
	genlog.ln \
	genextern.ln \
	gencommon.ln \
	comklog.ln \
	comstart.ln
COM_SRCS = $(COMMON_DIR)/volinfo.c \
	$(COMMON_DIR)/volmake.c \
	$(COMMON_DIR)/volmend.c \
	$(COMMON_DIR)/volplex.c \
	$(COMMON_DIR)/volsd.c \
	$(COMMON_DIR)/volume.c \
	$(COMMON_DIR)/genvolopts.c \
	$(COMMON_DIR)/genplxopts.c \
	$(COMMON_DIR)/genstart.c \
	$(COMMON_DIR)/genplexdet.c \
	$(COMMON_DIR)/genlog.c \
	$(COMMON_DIR)/genextern.c \
	$(COMMON_DIR)/gencommon.c \
	$(COMMON_DIR)/comklog.c \
	$(COMMON_DIR)/comstart.c

COM_HDRS = $(COMMON_DIR)/volgen.h \
	$(COMMON_DIR)/volswap.h
