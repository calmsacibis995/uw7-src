# @(#)cmd.vxvm:unixware/support/support.mk	1.6 10/10/97 15:58:59 - cmd.vxvm:unixware/support/support.mk
#ident	"@(#)cmd.vxvm:unixware/support/support.mk	1.6"

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

COMMON_DIR = ../../common/support
LINKDIR    = ../../unixware/support

include $(COMMON_DIR)/support.mk

include $(CMDRULES)

# Force cmdrules include from $(TOOLS) to $(ROOT)/$(MACH)
INC = $(ROOT)/$(MACH)/usr/include

# Pick a probe file as the key of restricted source
PROBEFILE = vxparms.c
MAKEFILE = support.mk

VXDIR	= $(ROOT)/$(MACH)/usr/lib/vxvm
SUPBIN	= $(VXDIR)/bin
SUPLIB	= $(VXDIR)/lib
DIAG_BIN= $(VXDIR)/diag.d
ETCSBIN = $(ROOT)/$(MACH)/etc/vx/sbin

LIBCMD_DIR = ../libcmd

LDLIBS = $(LIBCMD_DIR)/libcmd.a $(LIBVXVM) -lgen
# no lint library for gen
LINTLIB = -L$(LIBCMD_DIR) -llibcmd $(LIBVXVM) 
LOCALINC = $(COM_LOCALINC) -I$(LIBCMD_DIR) -I$(LINKDIR)
LOCALDEF = $(LOCAL)

# Re-define COM_BINTARGETS as the restricted SCP by splitting COM_BINTARGETS
# defined in $(COMMON_DIR)/support.mk into SCP_TARGETS which is derived
# from c sources and COM_BINTARGETS which is derived from shell scripts.
SCP_TARGETS = egettxt \
	vxcheckda \
	vxslicer \
	vxckdiskrm \
	vxspare \
	strtovoff

COM_BINTARGETS = vxbootsetup \
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

EGETTXT_OBJS = egettxt.o
EGETTXT_LINT_OBJS = egettxt.ln
EGETTXT_SRCS = egettxt.c

VCHKDA_OBJS = vxcheckda.o
VCHKDA_LINT_OBJS = vxcheckda.ln
VCHKDA_SRCS = vxcheckda.c

VSLICER_OBJS = vxslicer.o
VSLICER_LINT_OBJS = vxslicer.ln
VSLICER_SRCS = vxslicer.c

VCKDSKRM_OBJS = $(COM_VCKDSKRM_OBJS)
VCKDSKRM_LINT_OBJS = $(COM_VCKDSKRM_LINT_OBJS)
VCKDSKRM_SRCS = $(COM_VCKDSKRM_SRCS)

VSPARE_OBJS = vxspare.o
VSPARE_LINT_OBJS = vxspare.ln
VSPARE_SRCS = vxspare.c

VPARMS_OBJS = vxparms.o
VPARMS_LINT_OBJS = vxparms.ln
VPARMS_SRCS = vxparms.c

STRTOVOFF_OBJS = $(COM_STRTOVOFF_OBJS)
STRTOVOFF_LINT_OBJS = $(COM_STRTOVOFF_LINT_OBJS)
STRTOVOFF_SRCS = $(COM_STRTOVOFF_SRCS)

BINTARGETS = $(COM_BINTARGETS) $(SCP_TARGETS)

LIBTARGETS = $(COM_LIBTARGETS)

DIAGTARGETS = $(COM_DIAGTARGETS)

ETCSBINTARGETS= $(COM_ETCSBIN_TARGETS)

SUPPORT_TARGETS = $(COM_BINTARGETS) \
	$(COM_LIBTARGETS) \
	$(COM_DIAGTARGETS)

SUPPORT_OBJS = $(COM_OBJS)
SUPPORT_LINT_OBJS = $(COM_LINT_OBJS)

.c.o:
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<
	@if [ "$(@D)" != "./" ]; then ln -s $(LINKDIR)/$(@F) $*.o; fi

.IGNORE:

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(SCP_TARGETS) $(ETCSBINTARGETS) $(MAKEARGS) ; \
	else \
		for f in $(SCP_TARGETS) $(ETCSBINTARGETS) ; do \
			if [ ! -f $$f ]; then \
				echo "ERROR: $$f is missing" 1>&2 ;\
				false ;\
				break; \
			fi \
		done \
	fi

	$(MAKE) -f $(MAKEFILE) scripts $(MAKEARGS)

scripts: $(SUPPORT_TARGETS)

install: all
	[ -d $(VXDIR) ] || mkdir $(VXDIR)
	[ -d $(SUPBIN) ] || mkdir $(SUPBIN)
	[ -d $(SUPLIB) ] || mkdir $(SUPLIB)
	[ -d $(DIAG_BIN) ] || mkdir $(DIAG_BIN)
	[ -d $(ETCSBIN) ] || mkdir -p $(ETCSBIN)
	for f in $(BINTARGETS) ; \
	do \
		rm -f $(SUPBIN)/$$f; \
		$(INS) -f $(SUPBIN) -m 0555 -u root -g sys $$f ; \
	done
	for f in $(LIBTARGETS) ; \
	do \
		rm -f $(SUPLIB)/$$f; \
		$(INS) -f $(SUPLIB) -m 0555 -u root -g sys $$f ; \
	done
	for f in $(DIAGTARGETS) ; \
	do \
		rm -f $(DIAG_BIN)/$$f; \
		$(INS) -f $(DIAG_BIN) -m 0555 -u root -g sys $$f ; \
	done
	for f in $(ETCSBINTARGETS) ; \
	do \
		rm -f $(ETCSBIN)/$$f; \
		$(INS) -f $(ETCSBIN) -m 0555 -u root -g sys $$f ; \
	done
	rm -f $(SUPBIN)/vxswapcopy
	ln $(SUPBIN)/vxslicer $(SUPBIN)/vxswapcopy

	
lint:   $(SUPPORT_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(EGETTXT_LINT_OBJS)	
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VCHKDA_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VCKDSKRM_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VPARMS_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VSLICER_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VSPARE_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(STRTOVOFF_LINT_OBJS) 
	touch lint

egettxt: $(EGETTXT_OBJS)
	$(CC) -o $@ $(EGETTXT_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) \
		$(LDLIBS) $(NOSHLIBS)

vxcheckda: $(VCHKDA_OBJS)
	$(CC) -o $@ $(VCHKDA_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) \
		$(LDLIBS) $(NOSHLIBS)

vxckdiskrm: $(VCKDSKRM_OBJS)
	$(CC) -o $@ $(VCKDSKRM_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) \
		$(LDLIBS) $(SHLIBS)

vxparms: $(VPARMS_OBJS)
	$(CC) -o $@ $(VPARMS_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) \
		$(LDLIBS) $(NOSHLIBS)

vxslicer: $(VSLICER_OBJS)
	$(CC) -o $@ $(VSLICER_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) \
		$(LDLIBS) $(SHLIBS)

vxspare: $(VSPARE_OBJS)
	$(CC) -o $@ $(VSPARE_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) \
		$(LDLIBS) $(SHLIBS)

strtovoff: $(STRTOVOFF_OBJS)
	$(CC) -o $@ $(STRTOVOFF_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) \
		$(LDLIBS) $(SHLIBS)

headinstall:

clean:
	@rm -f $(SUPPORT_OBJS)
	@rm -f *.o

lintclean:
	rm -f $(SUPPORT_LINT_OBJS) lint

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		rm -f $(SCP_TARGETS) $(ETCSBINTARGETS); \
	fi
	rm -f $(SUPPORT_TARGETS)
	for f in $(SUPPORT_TARGETS) ; do \
		rm -f `basename $$f` ; \
	done
