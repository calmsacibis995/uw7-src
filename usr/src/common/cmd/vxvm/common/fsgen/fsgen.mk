# @(#)src/cmd/vxvm/common/fsgen/fsgen.mk	1.1 10/16/96 02:19:57 - 
#ident	"@(#)cmd.vxvm:common/fsgen/fsgen.mk	1.1"

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

COM_TARGETS = vxplex \
	vxresize

COM_VPLEX_OBJS = $(COMMON_GEN_DIR)/volplex.o \
	$(COMMON_GEN_DIR)/genextern.o \
	$(COMMON_GEN_DIR)/gencommon.o \
	$(COMMON_GEN_DIR)/genlog.o \
	$(COMMON_DIR)/fsplexdet.o \
	$(COMMON_DIR)/fsplxopts.o \
	$(COMMON_DIR)/cldopen.o

COM_VPLEX_LINT_OBJS = volplex.ln\
	fsplexdet.ln \
	fsplxopts.ln \
	genextern.ln \
	gencommon.ln \
	genlog.ln \
	cldopen.ln

COM_VPLEX_SRCS = $(COMMON_GEN_DIR)/volplex.c \
	$(COMMON_GEN_DIR)/genextern.c \
	$(COMMON_GEN_DIR)/gencommon.c \
	$(COMMON_GEN_DIR)/genlog.c \
	$(COMMON_DIR)/fsplexdet.c \
	$(COMMON_DIR)/fsplxopts.c \
	$(COMMON_DIR)/cldopen.c

COM_LOCALINC = -I../../common/libcmd \
	-I../../common/gen \
	-I../../common/fsgen

COM_LOCALDEF = -DUSE_TYPE='"fsgen"'

COM_OBJS = $(COMMON_DIR)/volplex.o \
	$(COMMON_DIR)/genextern.o \
	$(COMMON_DIR)/gencommon.o \
	$(COMMON_DIR)/genlog.o \
	$(COMMON_DIR)/fsplexdet.o \
	$(COMMON_DIR)/fsplxopts.o \
	$(COMMON_DIR)/cldopen.o

COM_SRCS = $(COMMON_DIR)/volplex.c \
	$(COMMON_DIR)/genextern.c \
	$(COMMON_DIR)/gencommon.c \
	$(COMMON_DIR)/genlog.c \
	$(COMMON_DIR)/fsplexdet.c \
	$(COMMON_DIR)/fsplxopts.c \
	$(COMMON_DIR)/cldopen.c

COM_HDRS = $(COMMON_DIR)/volfsgen.h \
	$(COMMON_DIR)/volroot.h


