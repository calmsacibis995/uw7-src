# @(#)src/cmd/vxvm/common/volassist/volassist.mk	1.1 10/16/96 02:24:18 - 
#ident	"@(#)cmd.vxvm:common/volassist/volassist.mk	1.1"

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

COMMON_DIR = ../../common/volassist

COM_TARGETS = vxassist

COM_VASSIST_OBJS = $(COMMON_DIR)/main.o \
	$(COMMON_DIR)/attr.o \
	$(COMMON_DIR)/config.o \
	$(COMMON_DIR)/license.o \
	$(COMMON_DIR)/mkobjects.o \
	$(COMMON_DIR)/parse.o \
	$(COMMON_DIR)/space.o \
	$(COMMON_DIR)/make.o \
	$(COMMON_DIR)/mirror.o \
	$(COMMON_DIR)/addlog.o \
	$(COMMON_DIR)/move.o \
	$(COMMON_DIR)/grow.o \
	$(COMMON_DIR)/shrink.o \
	$(COMMON_DIR)/snap.o \
	$(COMMON_DIR)/help.o \
	$(COMMON_DIR)/alloc.o \
	$(COMMON_DIR)/allocgroups.o \
	$(COMMON_DIR)/attriter.o \
	$(COMMON_DIR)/policies.o \
	$(COMMON_DIR)/p-extend.o \
	$(COMMON_DIR)/p-contig.o \
	$(COMMON_DIR)/p-general.o \
	$(COMMON_DIR)/p-regionlogs.o

COM_VASSIST_LINT_OBJS = main.ln \
	attr.ln \
	config.ln \
	license.ln \
	mkobjects.ln \
	parse.ln \
	space.ln \
	make.ln \
	mirror.ln \
	addlog.ln \
	move.ln \
	grow.ln \
	shrink.ln \
	snap.ln \
	help.ln \
	alloc.ln \
	allocgroups.ln \
	attriter.ln \
	policies.ln \
	p-extend.ln \
	p-contig.ln \
	p-general.ln \
	p-regionlogs.ln

COM_VASSIST_SRCS = $(COMMON_DIR)/main.c \
	$(COMMON_DIR)/attr.c \
	$(COMMON_DIR)/config.c \
	$(COMMON_DIR)/license.c \
	$(COMMON_DIR)/mkobjects.c \
	$(COMMON_DIR)/parse.c \
	$(COMMON_DIR)/space.c \
	$(COMMON_DIR)/make.c \
	$(COMMON_DIR)/mirror.c \
	$(COMMON_DIR)/addlog.c \
	$(COMMON_DIR)/move.c \
	$(COMMON_DIR)/grow.c \
	$(COMMON_DIR)/shrink.c \
	$(COMMON_DIR)/snap.c \
	$(COMMON_DIR)/help.c \
	$(COMMON_DIR)/alloc.c \
	$(COMMON_DIR)/allocgroups.c \
	$(COMMON_DIR)/attriter.c \
	$(COMMON_DIR)/policies.c \
	$(COMMON_DIR)/p-extend.c \
	$(COMMON_DIR)/p-contig.c \
	$(COMMON_DIR)/p-general.c \
	$(COMMON_DIR)/p-regionlogs.c

COM_LOCALINC = -I$(COMMON_DIR) -I../../common/libcmd

COM_OBJS = $(COM_VASSIST_OBJS)
COM_LINT_OBJS = $(COM_VASSIST_LINT_OBJS)

COM_SRCS = $(COM_VASSIST_SRCS)

COM_HDRS = 
