# @(#)src/cmd/vxvm/common/vold/vold.mk	1.1 10/16/96 02:26:30 - 
#ident	"@(#)cmd.vxvm:common/vold/vold.mk	1.1"

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

COMMON_DIR = ../../common/vold

COM_CONFIGD_TARGETS = vxconfigd
COM_DIAG_TARGETS = vxconfigdump \
	vxprivutil \
	vxkprint
COM_TEST_TARGETS = testpoll \
	vxwriteempty \
	testautoconfig

COM_TARGETS = $(COM_CONFIGD_TARGETS) $(COM_DIAG_TARGETS)

COM_CONFIGD_OBJS = $(COMMON_DIR)/change.o \
	$(COMMON_DIR)/check.o \
	$(COMMON_DIR)/client.o \
	$(COMMON_DIR)/coredb.o \
	$(COMMON_DIR)/da.o \
	$(COMMON_DIR)/dasup.o \
	$(COMMON_DIR)/dbase.o \
	$(COMMON_DIR)/dbcopy.o \
	$(COMMON_DIR)/dbsup.o \
	$(COMMON_DIR)/dg.o \
	$(COMMON_DIR)/dgimport.o \
	$(COMMON_DIR)/kernel.o \
	$(COMMON_DIR)/krecover.o \
	$(COMMON_DIR)/kernel_12.o \
	$(COMMON_DIR)/klog.o \
	$(COMMON_DIR)/logio.o \
	$(COMMON_DIR)/misc.o \
	$(COMMON_DIR)/mode.o \
	$(COMMON_DIR)/nopriv.o \
	$(COMMON_DIR)/priv.o \
	$(COMMON_DIR)/query.o \
	$(COMMON_DIR)/reqtbl.o \
	$(COMMON_DIR)/request.o \
	$(COMMON_DIR)/response.o \
	$(COMMON_DIR)/sliced.o \
	$(COMMON_DIR)/trans.o \
	$(COMMON_DIR)/volboot.o \
	$(COMMON_DIR)/voldb.o \
	$(COMMON_DIR)/voldbsup.o \
	$(COMMON_DIR)/voldctlop.o \
	$(COMMON_DIR)/voldctlsup.o

COM_CONFIGD_LINT_OBJS = change.ln \
	check.ln \
	client.ln \
	coredb.ln \
	da.ln \
	dasup.ln \
	dbase.ln \
	dbcopy.ln \
	dbsup.ln \
	dg.ln \
	dgimport.ln \
	kernel.ln \
	krecover.ln \
	kernel_12.ln \
	klog.ln \
	logio.ln \
	misc.ln \
	mode.ln \
	nopriv.ln \
	priv.ln \
	query.ln \
	reqtbl.ln \
	request.ln \
	response.ln \
	sliced.ln \
	trans.ln \
	volboot.ln \
	voldb.ln \
	voldbsup.ln \
	voldctlop.ln \
	voldctlsup.ln

COM_CONFIGD_SRCS = $(COMMON_DIR)/change.c \
	$(COMMON_DIR)/check.c \
	$(COMMON_DIR)/client.c \
	$(COMMON_DIR)/coredb.c \
	$(COMMON_DIR)/da.c \
	$(COMMON_DIR)/dasup.c \
	$(COMMON_DIR)/dbase.c \
	$(COMMON_DIR)/dbcopy.c \
	$(COMMON_DIR)/dbsup.c \
	$(COMMON_DIR)/dg.c \
	$(COMMON_DIR)/dgimport.c \
	$(COMMON_DIR)/kernel.c \
	$(COMMON_DIR)/krecover.c \
	$(COMMON_DIR)/kernel_12.c \
	$(COMMON_DIR)/klog.c \
	$(COMMON_DIR)/logio.c \
	$(COMMON_DIR)/misc.c \
	$(COMMON_DIR)/mode.c \
	$(COMMON_DIR)/nopriv.c \
	$(COMMON_DIR)/priv.c \
	$(COMMON_DIR)/query.c \
	$(COMMON_DIR)/reqtbl.c \
	$(COMMON_DIR)/request.c \
	$(COMMON_DIR)/response.c \
	$(COMMON_DIR)/sliced.c \
	$(COMMON_DIR)/trans.c \
	$(COMMON_DIR)/volboot.c \
	$(COMMON_DIR)/voldb.c \
	$(COMMON_DIR)/voldbsup.c \
	$(COMMON_DIR)/voldctlop.c \
	$(COMMON_DIR)/voldctlsup.c

COM_CONFIGDUMP_OBJS = $(COMMON_DIR)/dbdump.o \
	$(COMMON_DIR)/dbase.o \
	$(COMMON_DIR)/dbsup.o \
	$(COMMON_DIR)/voldbsup.o \
	$(COMMON_DIR)/misc.o

COM_CONFIGDUMP_LINT_OBJS = dbdump.ln \
	dbase.ln \
	dbsup.ln \
	voldbsup.ln \
	misc.ln

COM_CONFIGDUMP_SRCS = $(COMMON_DIR)/dbdump.c \
	$(COMMON_DIR)/dbase.c \
	$(COMMON_DIR)/dbsup.c \
	$(COMMON_DIR)/voldbsup.c \
	$(COMMON_DIR)/misc.c

COM_PRIVUTIL_OBJS = $(COMMON_DIR)/privutil.o \
	$(COMMON_DIR)/priv.o \
	$(COMMON_DIR)/dbsup.o \
	$(COMMON_DIR)/misc.o

COM_PRIVUTIL_LINT_OBJS = privutil.ln \
	priv.ln \
	dbsup.ln \
	misc.ln

COM_PRIVUTIL_SRCS = $(COMMON_DIR)/privutil.c \
	$(COMMON_DIR)/priv.c \
	$(COMMON_DIR)/dbsup.c \
	$(COMMON_DIR)/misc.c

COM_KPRINT_OBJS = $(COMMON_DIR)/volkprint.o
COM_KPRINT_LINT_OBJS = volkprint.ln
COM_KPRINT_SRCS = $(COMMON_DIR)/volkprint.c

COM_TESTPOLL_OBJS = testpoll.o \
	misc.o
COM_TESTPOLL_SRCS = $(COMMON_DIR)/testpoll.c \
	$(COMMON_DIR)/misc.c

COM_WRTEMPTY_OBJS = $(COMMON_DIR)/writeempty.o \
	$(COMMON_DIR)/dbase.o \
	$(COMMON_DIR)/dbsup.o \
	$(COMMON_DIR)/voldbsup.o \
	$(COMMON_DIR)/misc.o
COM_WRTEMPTY_SRCS = $(COMMON_DIR)/writeempty.c \
	$(COMMON_DIR)/dbase.c \
	$(COMMON_DIR)/dbsup.c \
	$(COMMON_DIR)/voldbsup.c \
	$(COMMON_DIR)/misc.c

COM_TESTAUTOCFG_OBJS = misc.o
COM_TESTAUTOCFG_SRCS = $(COMMON_DIR)/misc.c

COM_LOCALINC = -I$(COMMON_DIR) -I../../common/libcmd

COM_OBJS = $(COM_CONFIGD_OBJS) \
	$(COM_CONFIGDUMP_OBJS) \
	$(COM_PRIVUTIL_OBJS) \
	$(COM_KPRINT_OBJS)

COM_LINT_OBJS = $(COM_CONFIGD_LINT_OBJS) \
	$(COM_CONFIGDUMP_LINT_OBJS) \
	$(COM_PRIVUTIL_LINT_OBJS) \
	$(COM_KPRINT_LINT_OBJS)

COM_SRCS = $(COM_CONFIGD_SRCS) \
	$(COM_CONFIGDUMP_SRCS) \
	$(COM_PRIVUTIL_SRCS) \
	$(COM_KPRINT_SRCS)

COM_TEST_OBJS = testpoll.o \
	writeempty.o

COM_HDRS = 
