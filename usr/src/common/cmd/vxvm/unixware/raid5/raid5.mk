# @(#)cmd.vxvm:unixware/raid5/raid5.mk	1.6 10/10/97 15:58:57 - cmd.vxvm:unixware/raid5/raid5.mk
#ident	"@(#)cmd.vxvm:unixware/raid5/raid5.mk	1.6"

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

COMMON_DIR = ../../common/raid5
LINKDIR    = ../../unixware/raid5

include $(COMMON_DIR)/raid5.mk

include $(CMDRULES)

# Force cmdrules include from $(TOOLS) to $(ROOT)/$(MACH)
INC = $(ROOT)/$(MACH)/usr/include

# Pick a probe file as the key of restricted source
PROBEFILE = $(COMMON_DIR)/r5start.c
MAKEFILE = raid5.mk

LIBCMD_DIR = ../libcmd
GEN_DIR = ../gen

LDLIBS = $(LIBCMD_DIR)/libcmd.a $(LIBVXVM) -lgen
# no lint library for gen
LINTLIB = -L$(LIBCMD_DIR) -llibcmd $(LIBVXVM) 
LOCALINC = $(COM_LOCALINC) -I$(LIBCMD_DIR) -I$(GEN_DIR) -I$(LINKDIR)
LOCALDEF = $(COM_LOCALDEF) $(LOCAL)

VMAKE_OBJS = $(COM_VMAKE_OBJS)
VMAKE_LINT_OBJS = $(COM_VMAKE_LINT_OBJS)
VMAKE_SRCS = $(COM_VMAKE_SRCS)

VMEND_OBJS = $(COM_VMEND_OBJS)
VMEND_LINT_OBJS = $(COM_VMEND_LINT_OBJS)
VMEND_SRCS = $(COM_VMEND_SRCS)

VPLEX_OBJS = $(COM_VPLEX_OBJS)
VPLEX_LINT_OBJS = $(COM_VPLEX_LINT_OBJS)
VPLEX_SRCS = $(COM_VPLEX_SRCS)

VSD_OBJS = $(COM_VSD_OBJS)
VSD_LINT_OBJS = $(COM_VSD_LINT_OBJS)
VSD_SRCS = $(COM_VSD_SRCS)

VVOL_OBJS = $(COM_VVOL_OBJS)
VVOL_LINT_OBJS = $(COM_VVOL_LINT_OBJS)
VVOL_SRCS = $(COM_VVOL_SRCS)

R5_OBJS = $(COM_OBJS)
R5_LINT_OBJS = $(COM_LINT_OBJS)
R5_TARGETS = $(COM_TARGETS)
R5_HDRS = $(COM_HDRS)

VXDIR = $(ROOT)/$(MACH)/usr/lib/vxvm
VXTYPE = $(VXDIR)/type
VXR5 = $(VXTYPE)/raid5

.c.o:
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<
	@if [ "$(@D)" != "./" ]; then ln -s $(LINKDIR)/$(@F) $*.o; fi

.IGNORE:

all: headinstall
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(R5_TARGETS) $(MAKEARGS) ;\
	else \
		for f in $(R5_TARGETS); do \
			if [ ! -f $$f ]; then \
				echo "ERROR: $$f is missing" 1>&2 ;\
				false ;\
				break; \
			fi \
		done \
	fi

install: all
	@echo "Making install in cmd/vxvm/unixware/raid5"
	[ -d $(VXDIR) ] || mkdir -p $(VXDIR)
	[ -d $(VXTYPE) ] || mkdir -p $(VXTYPE)
	[ -d $(VXR5) ] || mkdir -p $(VXR5)
	for f in $(R5_TARGETS) ; \
	do \
		rm -f $(VXR5)/$$f ; \
		$(INS) -f $(VXR5) -m 0755 -u root -g sys $$f ; \
	done

lint:   $(R5_LINT_OBJS)
	$(LINT) $(LINT_LINK_FLAGS) $(LINTFLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VMAKE_LINT_OBJS)
	$(LINT) $(LINT_LINK_FLAGS) $(LINTFLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VMEND_LINT_OBJS)
	$(LINT) $(LINT_LINK_FLAGS) $(LINTFLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VPLEX_LINT_OBJS)
	$(LINT) $(LINT_LINK_FLAGS) $(LINTFLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VSD_LINT_OBJS)
	$(LINT) $(LINT_LINK_FLAGS) $(LINTFLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VVOL_LINT_OBJS)
	touch lint

headinstall:
	@echo "Making headinstall in cmd/vxvm/unixware/raid5"
	for f in $(R5_HDRS) ; \
	do \
		$(INS) -f $(INC) -m 0644 -u root -g sys $$f ; \
	done

clean:
	@echo "Making clean in cmd/vxvm/unixware/raid5"
	rm -f $(R5_OBJS)
	rm -f *.o

lintclean:
	rm -f $(R5_LINT_OBJS) lint

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		rm -f $(R5_TARGETS); \
	fi

vxmake: $(VMAKE_OBJS)
	$(CC) -o $@ $(VMAKE_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(LDLIBS)

vxmend: $(VMEND_OBJS)
	$(CC) -o $@ $(VMEND_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(LDLIBS)

vxplex: $(VPLEX_OBJS)
	$(CC) -o $@ $(VPLEX_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(LDLIBS)

vxsd: $(VSD_OBJS)
	$(CC) -o $@ $(VSD_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(LDLIBS)

vxvol: $(VVOL_OBJS)
	$(CC) -o $@ $(VVOL_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(LDLIBS)

gencommon.o: $(COMMON_GEN_DIR)/gencommon.c
	$(CC) $(CFLAGS) $(DEFLIST) -c $(COMMON_GEN_DIR)/gencommon.c

genextern.o: $(COMMON_GEN_DIR)/genextern.c
	$(CC) $(CFLAGS) $(DEFLIST) -c $(COMMON_GEN_DIR)/genextern.c

genlog.o: $(COMMON_GEN_DIR)/genlog.c
	$(CC) $(CFLAGS) $(DEFLIST) -c $(COMMON_GEN_DIR)/genlog.c

comklog.o: $(COMMON_GEN_DIR)/comklog.c
	$(CC) $(CFLAGS) $(DEFLIST) -c $(COMMON_GEN_DIR)/comklog.c

#lint objects
gencommon.ln: $(COMMON_GEN_DIR)/gencommon.c
	$(LINT) $(LINTFLAGS) $(CFLAGS)  $(DEFLIST) -c \
	$(COMMON_GEN_DIR)/gencommon.c
 
genextern.ln: $(COMMON_GEN_DIR)/genextern.c
	$(LINT) $(LINTFLAGS) $(CFLAGS)  $(DEFLIST) -c \
	$(COMMON_GEN_DIR)/genextern.c
 
genlog.ln: $(COMMON_GEN_DIR)/genlog.c
	$(LINT) $(LINTFLAGS) $(CFLAGS)  $(DEFLIST) -c \
	$(COMMON_GEN_DIR)/genlog.c
comklog.ln: $(COMMON_GEN_DIR)/comklog.c
	$(LINT) $(LINTFLAGS) $(CFLAGS)  $(DEFLIST) -c \
	$(COMMON_GEN_DIR)/comklog.c

.NO_PARALLEL:	headinstall


