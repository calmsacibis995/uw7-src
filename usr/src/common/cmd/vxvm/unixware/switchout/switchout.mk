# @(#)cmd.vxvm:unixware/switchout/switchout.mk	1.6 10/10/97 15:59:00 - cmd.vxvm:unixware/switchout/switchout.mk
#ident	"@(#)cmd.vxvm:unixware/switchout/switchout.mk	1.6"

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

COMMON_DIR = ../../common/switchout
LINKDIR    = ../../unixware/switchout

include $(COMMON_DIR)/switchout.mk

include $(CMDRULES)

# Force cmdrules include from $(TOOLS) to $(ROOT)/$(MACH)
INC = $(ROOT)/$(MACH)/usr/include

# Pick a probe file as the key of restricted source
PROBEFILE = $(COMMON_DIR)/volmake.c
MAKEFILE = switchout.mk

LIBCMD_DIR = ../libcmd

LDLIBS = $(LIBCMD_DIR)/libcmd.a $(LIBVXVM) -lgen
# no lint library for gen
LINTLIB = -L$(LIBCMD_DIR) -llibcmd $(LIBVXVM) 
LOCALINC = $(COM_LOCALINC) -I$(LIBCMD_DIR) -I$(LINKDIR)
#LOCALDEF = -DUW_2_1 $(I18N)
LOCALDEF = $(LOCAL)

VINFO_OBJS = $(COM_VINFO_OBJS)
VINFO_LINT_OBJS = $(COM_VINFO_LINT_OBJS)
VINFO_SRCS = $(COM_VINFO_SRCS)

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

SWITCHOUT_OBJS = $(COM_OBJS)
SWITCHOUT_LINT_OBJS = $(COM_LINT_OBJS)
SWITCHOUT_SRCS = $(COM_SRCS)
SWITCHOUT_HDRS = $(COM_HDRS)

SWITCHOUT_TARGETS = $(COM_TARGETS)

.c.o:
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<
	@if [ "$(@D)" != "./" ]; then ln -s $(LINKDIR)/$(@F) $*.o; fi

.IGNORE:

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(SWITCHOUT_TARGETS) $(MAKEARGS) ;\
	else \
		for f in $(SWITCHOUT_TARGETS); do \
			if [ ! -f $$f ]; then \
				echo "ERROR: $$f is missing" 1>&2 ;\
				false ;\
				break; \
			fi \
		done \
	fi

install: all
	for f in $(SWITCHOUT_TARGETS) ; \
	do \
		rm -f $(USRSBIN)/$$f ; \
		$(INS) -f $(USRSBIN) -m 0755 -u root -g sys $$f ; \
	done

lint: 	$(SWITCHOUT_LINT_OBJS)
	$(LINT) $(LINT_LINK_FLAGS) $(LINTFLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VINFO_LINT_OBJS)
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
	@echo "Making headinstall in cmd/vxvm/unixware/switchout"

clean:
	@echo "Making clean in cmd/vxvm/unixware/switchout"
	rm -f $(SWITCHOUT_OBJS)
	rm -f *.o

lintclean:
	@echo "Making lintclean in cmd/vxvm/unixware/switchout"
	rm -f $(SWITCHOUT_LINT_OBJS) lint

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "Making clobber in cmd/vxvm/unixware/switchout" ; \
		rm -f $(SWITCHOUT_TARGETS) ; \
	fi

vxinfo: $(VINFO_OBJS)
	$(CC) -o $@ $(VINFO_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(LDLIBS)

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
