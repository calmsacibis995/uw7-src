# @(#)src/uw/cmd/vxfs/vxrestore/vxrestore.mk	3.6.9.1 11/08/97 12:53:12 - 
#ident "@(#)vxfs:src/uw/cmd/vxfs/vxrestore/vxrestore.mk	3.6.9.1"
#ident	"@(#)vxfs.cmds:common/cmd/fs.d/vxfs/vxrestore/vxrestore.mk	1.2.3.1"
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

include $(CMDRULES)

LIBDIR = ../libvxfs
LDLIBS = -L$(LIBDIR) -lvxfs -lgen
LOCALINC = -I../include
LOCALDEF = $(LARGEFILES)
INSDIR1 = $(USRLIB)/fs/vxfs
INSDIR2 = $(USRSBIN)
OWN = bin
GRP = bin
RMTSTUB = dumpstubs.o
RMTOBJS = dumprmt.o
RMTLIBS = -lsocket -lnsl
OBJECTS = \
	dirs.o \
	interact.o \
	machdep.o \
	main.o \
	restore.o \
	symtab.o \
	tape.o \
	utilities.o

PROBEFILE = dirs.c
MAKEFILE = vxrestore.mk
BINARIES = vxrestore.dy vxrestore

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
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) vxrestore.dy
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) vxrestore
	$(CH)-rm -f $(INSDIR2)/vxrestore
	$(CH)ln $(INSDIR1)/vxrestore.dy $(INSDIR2)/vxrestore

clean:
	-rm -f $(OBJECTS) $(RMTOBJS) $(RMTSTUB)

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

binaries: $(BINARIES)

rvxrestore: $(RMTOBJS) $(OBJECTS) $(LIBDIR)/libvxfs.a
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(RMTOBJS) \
		$(LDLIBS) $(SHLIBS) $(RMTLIBS)

vxrestore.dy: $(RMTSTUB) $(OBJECTS) $(LIBDIR)/libvxfs.a
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(RMTSTUB) $(LDLIBS) $(SHLIBS)

vxrestore: $(OBJECTS) $(RMTSTUB) $(LIBDIR)/libvxfs.a
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(RMTSTUB) $(LDLIBS) $(NOSHLIBS)

dirs.o:		dirs.c

interact.o:	interact.c

machdep.o:	machdep.c

main.o:		main.c

restore.o:	restore.c

symtab.o:	symtab.c

tape.o:		tape.c

utilities.o:	utilities.c

dumprmt.o:	dumprmt.c

dumpstubs.o:	dumpstubs.c
