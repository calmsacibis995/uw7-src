# @(#)src/uw/cmd/vxfs/vxdump/vxdump.mk	3.6.9.1 11/08/97 12:53:05 - 
#ident	"@(#)vxfs:src/uw/cmd/vxfs/vxdump/vxdump.mk	3.6.9.1"
#ident	"@(#)vxfs.cmds:common/cmd/fs.d/vxfs/vxdump/vxdump.mk	1.2.3.1"
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
RMTLIBS = -lsocket -lnsl
LOCALINC = -I../include
LOCALDEF = $(LARGEFILES)

INSDIR1 = $(USRLIB)/fs/vxfs
INSDIR2 = $(USRSBIN)
OWN = bin
GRP = bin
RMTOBJS = dumprmt.o
RMTSTUB = dumpstubs.o
OBJECTS = \
	itime.o \
	main.o \
	machdep.o \
	optr.o \
	tape.o \
	traverse.o \
	unctime.o

INCLUDES = dump.h

PROBEFILE = main.c
MAKEFILE = vxdump.mk
BINARIES = vxdump

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
	$(CH)-rm -f $(INSDIR2)/vxdump
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) vxdump
	$(CH)-ln $(INSDIR1)/vxdump $(INSDIR2)/vxdump

clean:
	-rm -f $(OBJECTS) $(RMTOBJECTS) $(RMTSTUB)

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

binaries: $(BINARIES)

rvxdump: $(OBJECTS) $(RMTOBJECTS) $(LIBDIR)/libvxfs.a
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(RMTOBJECTS) $(LDLIBS) \
		$(RMTLIBS) $(SHLIBS)

vxdump: $(OBJECTS) $(RMTSTUB) $(LIBDIR)/libvxfs.a
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(RMTSTUB) $(LDLIBS) $(SHLIBS)

itime.o: itime.c $(INCLUDES)

main.o: main.c $(INCLUDES)

machdep.o: machdep.c $(INCLUDES)

optr.o: optr.c $(INCLUDES)

tape.o: tape.c $(INCLUDES)

traverse.o: traverse.c $(INCLUDES)

unctime.o: unctime.c $(INCLUDES)

dumprmt.o: dumprmt.c $(INCLUDES)

dumpstubs.o: dumpstubs.c $(INCLUDES)






