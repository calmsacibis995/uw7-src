# @(#)src/uw/cmd/vxfs/fsdb/fsdb.mk	3.6.9.1 11/08/97 12:52:51 - 
#ident "@(#)vxfs:src/uw/cmd/vxfs/fsdb/fsdb.mk	3.6.9.1"
#ident	"@(#)vxfs.cmds:common/cmd/fs.d/vxfs/fsdb/fsdb.mk	1.2.2.1"
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

#
# If you have the GNU readline library laying around, fsdb can be built
# to use it.  Uncomment the following lines and change the paths to point
# to the location of the readline libraries and includes.
#
#DEVDEF = -DREADLINE -DHAVE_STRING_H
#DEVINC = -I/usr/local/src/cmd
#DEVLIBS = -L /usr/local/lib -lreadline -lcurses

LIBDIR = ../libvxfs
LDLIBS = $(DEVLIBS) -L$(LIBDIR) -lvxfs -lgen
INCDIR = ../include
LOCALINC = -I$(INCDIR) $(DEVINC)
LOCALDEF = $(LARGEFILES)

INSDIR1 = $(USRLIB)/fs/vxfs
INSDIR2 = $(ETC)/fs/vxfs
OWN = bin
GRP = bin

PROBEFILE = fsdb.c
MAKEFILE = fsdb.mk
BINARIES = fsdb
OBJECTS = fsdb.o

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
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fsdb
	$(CH)$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fsdb

clean:
	-rm -f $(OBJECTS)

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

binaries: $(BINARIES)

fsdb: $(OBJECTS) $(LIBDIR)/libvxfs.a
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS $(NOSHLIBS) $(LDLIBS)

fsdb.o: fsdb.c
