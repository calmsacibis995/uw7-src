# @(#)src/cmd/vxvm/unixware/fsgen/fs/vxfs/vxfs.mk	1.1 10/16/96 02:17:26 - 
#ident	"@(#)cmd.vxvm:unixware/fsgen/fs/vxfs/vxfs.mk	1.5"

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

include $(CMDRULES)

# Force cmdrules include from $(TOOLS) to $(ROOT)/$(MACH)
INC = $(ROOT)/$(MACH)/usr/include

# Pick a probe file as the key of restricted source
PROBEFILE = volsync.c
MAKEFILE = vxfs.mk

COM_LIBCMD_DIR = ../../../../common/libcmd
LIBCMD_DIR = ../../../libcmd

LOCALINC = -I$(COM_LIBCMD_DIR) -I$(LIBCMD_DIR)
LOCALDEF = $(LOCAL)
LDLIBS = $(LIBVXVM) -lgen

VXDIR = $(ROOT)/$(MACH)/usr/lib/vxvm
VXTYPE = $(VXDIR)/type
VXFSGEN = $(VXTYPE)/fsgen
VXFSFS = $(VXFSGEN)/fs.d
VXVXFS = $(VXFSFS)/vxfs

VXFS_TARGETS = vxsync vxresize

VXSYNC_OBJS = volsync.o
VXSYNC_LINT_OBJS = volsync.ln
VXSYNC_SRCS = volsync.c

VXFS_OBJECTS = $(VXSYNC_OBJS)

.sh:
	cat $< > $@; chmod +x $@

.IGNORE:

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(VXFS_TARGETS) $(MAKEARGS) ;\
	else \
		for f in $(VXFS_TARGETS); do \
			if [ ! -f $$f ]; then \
				echo "ERROR: $$f is missing" 1>&2 ;\
				false ;\
				break; \
			fi \
		done \
	fi

vxsync: $(VXSYNC_OBJS)
	$(CC) -o $@ $(VXSYNC_OBJS) $(LDFLAGS) $(LDLIBS)

lint:	$(VXSYNC_LINT_OBJS)
	$(LINT) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LDLIBS) $(VXSYNC_LINT_OBJS)
	touch lint

vxresize: volresize.sh
	cat volresize.sh > $@
	chmod +x $@

install: all
	[ -d $(VXDIR) ] || mkdir -p $(VXDIR)
	[ -d $(VXTYPE) ] || mkdir -p $(VXTYPE)
	[ -d $(VXFSGEN) ] || mkdir -p $(VXFSGEN)
	[ -d $(VXFSFS) ] || mkdir -p $(VXFSFS)
	[ -d $(VXVXFS) ] || mkdir -p $(VXVXFS)
	for f in $(VXFS_TARGETS) ; \
	do \
		rm -f $(VXVXFS)/$$f ; \
		$(INS) -f $(VXVXFS) -m 0755 -u root -g sys $$f ; \
	done

headinstall:

clean:
	rm -f $(VXFS_OBJECTS)

lintclean:
	rm -f $(VXSYNC_LINT_OBJS) lint

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		rm -f $(VXFS_TARGETS) ; \
	fi

