# @(#)cmd.vxvm:unixware/gen/gen.mk	1.6 10/10/97 15:55:36 - cmd.vxvm:unixware/gen/gen.mk
#ident	"@(#)cmd.vxvm:unixware/gen/gen.mk	1.6"

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

COMMON_DIR = ../../common/gen
LINKDIR    = ../../unixware/gen

include $(COMMON_DIR)/gen.mk

include $(CMDRULES)

# Force cmdrules include from $(TOOLS) to $(ROOT)/$(MACH)
INC = $(ROOT)/$(MACH)/usr/include

# Pick a probe file as the key of restricted source
PROBEFILE = $(COMMON_DIR)/genstart.c
MAKEFILE = gen.mk

LIBCMD_DIR = ../libcmd
VOLD_DIR = ../vold
FSGEN_DIR = ../fsgen

LDLIBS = $(LIBCMD_DIR)/libcmd.a $(LIBVXVM) -lgen

# there is no lint library for gen and lint library name for libcmd will 
# be libcmd.ln
LINTLIB = -L$(LIBCMD_DIR) -llibcmd $(LIBVXVM) 
LOCALINC = $(COM_LOCALINC) -I$(LIBCMD_DIR) -I$(FSGEN_DIR) -I$(LINKDIR) -I$(COMMON_DIR)
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

GEN_OBJS = $(COM_OBJS)
GEN_LINT_OBJS = $(COM_LINT_OBJS)
GEN_TARGETS = $(COM_TARGETS)
GEN_HDRS = $(COM_HDRS)

R5_LINKS = $(COM_R5_LINKS)
FSGEN_LINKS = $(COM_FSGEN_LINKS)

VXDIR = $(ROOT)/$(MACH)/usr/lib/vxvm
VXTYPE = $(VXDIR)/type

.c.o:
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<
	@if [ "$(@D)" != "./" ]; then ln -s $(LINKDIR)/$(@F) $*.o; fi

.IGNORE:

all: headinstall
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(GEN_TARGETS) $(MAKEARGS) ;\
	else \
		for f in $(GEN_TARGETS); do \
			if [ ! -f $$f ]; then \
				echo "ERROR: $$f is missing" 1>&2 ;\
				false ;\
				break; \
			fi \
		done \
	fi

install: all
	[ -d $(VXDIR) ] || mkdir -p $(VXDIR)
	[ -d $(VXTYPE) ] || mkdir -p $(VXTYPE)
	[ -d $(VXTYPE)/gen ] || mkdir -p $(VXTYPE)/gen
	[ -d $(VXTYPE)/fsgen ] || mkdir -p $(VXTYPE)/fsgen
	[ -d $(VXTYPE)/raid5 ] || mkdir -p $(VXTYPE)/raid5
	rm -rf $(VXTYPE)/swap
	ln -s gen $(VXTYPE)/swap

	for f in $(GEN_TARGETS) ; \
	do \
		rm -f $(VXTYPE)/gen/$$f ; \
		$(INS) -f $(VXTYPE)/gen -m 0755 -u root -g sys $$f ; \
	done
	for f in $(FSGEN_LINKS) ; \
	do \
		rm -f $(VXTYPE)/fsgen/$$f ; \
		ln -s ../gen/$$f $(VXTYPE)/fsgen/$$f ; \
	done
	for f in $(R5_LINKS) ; \
	do \
		rm -f $(VXTYPE)/raid5/$$f ; \
		ln -s ../gen/$$f $(VXTYPE)/raid5/$$f ; \
	done

lint:	$(GEN_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VINFO_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VMAKE_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR)  \
	$(LINTLIB) $(VMEND_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VPLEX_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VSD_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VVOL_LINT_OBJS)
	touch lint

headinstall:
	@echo "Making headinstall in cmd/vxvm/unixware/gen" ; \
	for f in $(GEN_HDRS) ; \
	do \
		$(INS) -f $(INC) -m 0644 -u root -g sys $$f ; \
	done

clean:
	@echo "Making clean in cmd/vxvm/unixware/gen"
	rm -f $(GEN_OBJS)
	rm -f *.o

lintclean:
	rm -f $(GEN_LINT_OBJS) lint

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "Making clobber in cmd/vxvm/unixware/gen" ; \
		rm -f $(GEN_TARGETS) ; \
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

.NO_PARALLEL:	headinstall


