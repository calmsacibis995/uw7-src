# @(#)src/uw/cmd/vxfs/fsck/fsck.mk	3.5.9.1 11/08/97 12:52:50 - 
#ident	"@(#)vxfs:src/uw/cmd/vxfs/fsck/fsck.mk	3.5.9.1"
#ident	"@(#)vxfs.cmds:common/cmd/fs.d/vxfs/fsck/fsck.mk	1.1.2.1"
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

LIBDIR	= ../libvxfs
LDLIBS	= -L$(LIBDIR) -lvxfsi -lgen
LOCALINC= -I../include
LOCALDEF = $(LARGEFILES) -DVXI18N
INSDIR1 = $(USRLIB)/fs/vxfs
INSDIR2 = $(ETC)/fs/vxfs
OWN = bin
GRP = bin

OBJECTS	= \
	aggr.o \
	attr.o \
	bmap.o \
	dir.o \
	extent.o \
	extern.o \
	fset.o \
	inode.o \
	links.o \
	lwrite.o \
	machdep.o \
	main.o \
	olt.o \
	readi.o \
	replay.o \
	subr.o \
	subreplay.o \
	super.o

PROBEFILE = dir.c
MAKEFILE = fsck.mk
BINARIES = fsck fsck.dy

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
	$(CH)$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fsck
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fsck
	$(CH)$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fsck.dy
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fsck.dy

clean:
	-rm -f $(OBJECTS)

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

binaries: $(BINARIES)

fsck: $(OBJECTS) $(LIBDIR)/libvxfsi.a
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS) $(ROOTLIBS)

fsck.dy: $(OBJECTS) $(LIBDIR)/libvxfsi.a
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS) $(SHLIBS)

aggr.o: aggr.c fsck.h proto.h

attr.o: attr.c fsck.h proto.h

bmap.o: bmap.c fsck.h proto.h

dir.o: dir.c fsck.h proto.h

extent.o: extent.c fsck.h proto.h

extern.o: extern.c fsck.h proto.h

fset.o: fset.c fsck.h proto.h

inode.o: inode.c fsck.h proto.h

links.o: links.c fsck.h proto.h

lwrite.o: lwrite.c fsck.h proto.h

machdep.o: machdep.c fsck.h proto.h

main.o: main.c fsck.h proto.h

olt.o: olt.c fsck.h proto.h

readi.o: readi.c fsck.h proto.h

replay.o: replay.c fsck.h proto.h replay.h

subr.o: subr.c fsck.h proto.h

subreplay.o: subreplay.c fsck.h proto.h replay.h

super.o: super.c fsck.h proto.h
