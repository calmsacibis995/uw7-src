# @(#)cmd.vxvm:unixware/admin/admin.mk	1.6 10/10/97 15:55:33 - cmd.vxvm:unixware/admin/admin.mk
#ident	"@(#)cmd.vxvm:unixware/admin/admin.mk	1.6"

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

COMMON_DIR = ../../common/admin
LINKDIR    = ../../unixware/admin

include $(COMMON_DIR)/admin.mk

include $(CMDRULES)

# Force cmdrules include from $(TOOLS) to $(ROOT)/$(MACH)
INC = $(ROOT)/$(MACH)/usr/include

# Pick a probe file as the key of restricted source
PROBEFILE = $(COMMON_DIR)/voldctl.c
MAKEFILE = admin.mk

LIBCMD = -L../libcmd -lcmd 

LDLIBS = $(LIBCMD) $(LIBVXVM) -lgen
LOCALINC = $(COM_LOCALINC) -I. -I../../common/libcmd -I$(LINKDIR)
#LOCALDEF = -DUW_2_1 $(I18N)
LOCALDEF = $(LOCAL)

# there is no lint library for gen and lint library name for libcmd will 
# be libcmd.ln
LINTLIB = $(LIBCMD) $(LIBVXVM)

VEDIT_OBJS = $(COM_VEDIT_OBJS)
VEDIT_SRCS = $(COM_VEDIT_SRCS)
VEDIT_LINT_OBJS = $(COM_VEDIT_LINT_OBJS)

VPRINT_OBJS = $(COM_VPRINT_OBJS)
VPRINT_SRCS = $(COM_VPRINT_SRCS)
VPRINT_LINT_OBJS = $(COM_VPRINT_LINT_OBJS)

VDISK_OBJS = $(COM_VDISK_OBJS)
VDISK_SRCS = $(COM_VDISK_SRCS)
VDISK_LINT_OBJS = $(COM_VDISK_LINT_OBJS)

VDG_OBJS = $(COM_VDG_OBJS)
VDG_SRCS = $(COM_VDG_SRCS)
VDG_LINT_OBJS = $(COM_VDG_LINT_OBJS)

VNOTIFY_OBJS = $(COM_VNOTIFY_OBJS)
VNOTIFY_SRCS = $(COM_VNOTIFY_SRCS)
VNOTIFY_LINT_OBJS = $(COM_VNOTIFY_LINT_OBJS)

VDCTL_OBJS = $(COM_VDCTL_OBJS)
VDCTL_SRCS = $(COM_VDCTL_SRCS)
VDCTL_LINT_OBJS = $(COM_VDCTL_LINT_OBJS)

VIOD_OBJS = $(COM_VIOD_OBJS)
VIOD_SRCS = $(COM_VIOD_SRCS)
VIOD_LINT_OBJS = $(COM_VIOD_LINT_OBJS)

VRECOVER_OBJS = $(COM_VRECOVER_OBJS)
VRECOVER_SRCS = $(COM_VRECOVER_SRCS)
VRECOVER_LINT_OBJS = $(COM_VRECOVER_LINT_OBJS)

VTRACE_OBJS = $(COM_VTRACE_OBJS)
VTRACE_SRCS = $(COM_VTRACE_SRCS)
VTRACE_LINT_OBJS = $(COM_VTRACE_LINT_OBJS)

VSTAT_OBJS = $(COM_VSTAT_OBJS)
VSTAT_SRCS = $(COM_VSTAT_SRCS)
VSTAT_LINT_OBJS = $(COM_VSTAT_LINT_OBJS)

ADMIN_OBJS = $(COM_OBJS)
ADMIN_TARGETS = $(COM_TARGETS)
ADMIN_SBIN_TARGETS = $(COM_SBIN_TARGETS)
ADMIN_LINT_OBJS = $(COM_LINT_OBJS)

.c.o:
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<
	@if [ "$(@D)" != "./" ]; then ln -s $(LINKDIR)/$(@F) $*.o; fi

.IGNORE:

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(ADMIN_TARGETS) $(ADMIN_SBIN_TARGETS) \
			$(MAKEARGS) ;\
	else \
		for f in $(ADMIN_TARGETS) $(ADMIN_SBIN_TARGETS); do \
			if [ ! -f $$f ]; then \
				echo "ERROR: $$f is missing" 1>&2 ;\
				false ;\
				break; \
			fi \
		done \
	fi

install: all
	for f in $(ADMIN_TARGETS) ; \
	do \
		rm -f $(USRSBIN)/$$f ; \
		$(INS) -f $(USRSBIN) -m 0755 -u root -g sys $$f ; \
	done
	for f in $(ADMIN_SBIN_TARGETS) ; \
	do \
		rm -f $(SBIN)/$$f ; \
		$(INS) -f $(SBIN) -m 0755 -u root -g sys $$f ; \
	done

lint:   $(ADMIN_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VEDIT_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VPRINT_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VDISK_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VDG_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VNOTIFY_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VRECOVER_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VSTAT_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VDCTL_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VIOD_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VTRACE_LINT_OBJS)
	touch lint

headinstall:

clean:
	@rm -f $(ADMIN_OBJS)
	@rm -f *.o

lintclean:
	rm -f $(ADMIN_LINT_OBJS) lint
	

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		rm -f $(ADMIN_TARGETS) $(ADMIN_SBIN_TARGETS) ;\
	fi

vxedit: $(VEDIT_OBJS)
	$(CC) -o $@ $(VEDIT_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(LDLIBS)

vxprint: $(VPRINT_OBJS)
	$(CC) -o $@ $(VPRINT_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(LDLIBS)

vxdisk: $(VDISK_OBJS)
	$(CC) -o $@ $(VDISK_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(LDLIBS)

vxdg: $(VDG_OBJS)
	$(CC) -o $@ $(VDG_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(LDLIBS)

vxnotify: $(VNOTIFY_OBJS)
	$(CC) -o $@ $(VNOTIFY_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(LDLIBS)

vxrecover: $(VRECOVER_OBJS)
	$(CC) -o $@ $(VRECOVER_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(LDLIBS)

vxstat: $(VSTAT_OBJS)
	$(CC) -o $@ $(VSTAT_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(LDLIBS)

vxdctl: $(VDCTL_OBJS)
	$(CC) -o $@ $(VDCTL_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(LDLIBS)

vxiod: $(VIOD_OBJS)
	$(CC) -o $@ $(VIOD_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(LDLIBS)

vxtrace: $(VTRACE_OBJS)
	$(CC) -o $@ $(VTRACE_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(LDLIBS)



