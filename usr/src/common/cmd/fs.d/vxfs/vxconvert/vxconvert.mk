# @(#)src/uw/cmd/vxfs/vxconvert/vxconvert.mk	3.8.9.1 11/08/97 12:53:02 - 
#ident	"@(#)vxfs:src/uw/cmd/vxfs/vxconvert/vxconvert.mk	3.8.9.1"
#ident	"@(#)vxfs.cmds:common/cmd/fs.d/vxfs/vxconvert/vxconvert.mk	1.2.3.1"
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
LDLIBS = -L$(LIBDIR) -lvxfs -lgen
LOCALINC= -I../include -I.
LOCALDEF = $(LARGEFILES)
INSDIR = $(USRSBIN)
OWN = bin
GRP = bin

OBJECTS	= \
	balloc.o \
	bcache.o \
	bmap.o \
	dir.o \
	inode.o \
	machdep.o \
	main.o \
	misc.o \
	setup.o \
	sfs.o \
	smap.o

PROBEFILE = dir.c
MAKEFILE = vxconvert.mk
BINARIES = vxconvert

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
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(CH)$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) vxconvert
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) vxconvert

clean:
	-rm -f $(OBJECTS)

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

binaries: $(BINARIES)

vxconvert: $(OBJECTS) $(LIBDIR)/libvxfsi.a
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS) $(ROOTLIBS)

balloc.o: balloc.c

bcache.o: bcache.c

bmap.o: bmap.c

dir.o: dir.c

inode.o: inode.c

machdep.o: machdep.c

main.o: main.c

misc.o: misc.c

setup.o: setup.c

sfs.o: sfs.c

smap.o: smap.c
