# @(#)src/uw/cmd/vxfs/mkfs/mkfs.mk	3.6.9.1 11/08/97 12:52:56 - 
#ident	"@(#)vxfs:src/uw/cmd/vxfs/mkfs/mkfs.mk	3.6.9.1"
#ident	"@(#)vxfs.cmds:common/cmd/fs.d/vxfs/mkfs/mkfs.mk	1.1.4.1"
#
# Copyright (c) 1996 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
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
#	       RESTRICTED RIGHTS LEGEND
# USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
# SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
# (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
# COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
#	       VERITAS SOFTWARE
# 1600 PLYMOUTH STREET, MOUNTAIN VIEW, CA 94043
#

include $(CMDRULES)

LIBDIR = ../libvxfs
LDLIBS = -L$(LIBDIR) -lvxfsi -lgen
INCDIR = ../include
LOCALINC = -I$(INCDIR)
LOCALDEF = $(LARGEFILES) -DVXI18N

INSDIR1 = $(USRLIB)/fs/vxfs
INSDIR2 = $(ETC)/fs/vxfs
OWN = bin
GRP = bin

PROBEFILE = mkfs.c
MAKEFILE = mkfs.mk
BINARIES = mkfs mkfs.dy

OBJECTS = mkfs.o \
	machdep.o \
	tables.o

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) ;\
	else \
		for fl in $(BINARIES); do \
			if [ ! -f $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi

install: all
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(CH)$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mkfs
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mkfs
	$(CH)$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mkfs.dy
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mkfs.dy

clean:
	-rm -f $(OBJECTS)

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

binaries: $(BINARIES)

mkfs: $(OBJECTS) $(LIBDIR)/libvxfsi.a
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS) $(ROOTLIBS)

mkfs.dy: $(OBJECTS) $(LIBDIR)/libvxfsi.a
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS) $(SHLIBS)

mkfs.o: mkfs.c

machdep.o: machdep.c

tables.o: tables.c
