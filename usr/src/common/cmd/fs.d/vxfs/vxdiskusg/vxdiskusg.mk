# @(#)src/uw/cmd/vxfs/vxdiskusg/vxdiskusg.mk	3.6.9.1 11/08/97 12:53:03 - 
#ident	"@(#)vxfs:src/uw/cmd/vxfs/vxdiskusg/vxdiskusg.mk	3.6.9.1"
#ident	"@(#)vxfs.cmds:common/cmd/fs.d/vxfs/vxdiskusg/vxdiskusg.mk	1.1.2.1"
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
ACCT_DIR = $(ROOT)/usr/src/$(WORK)/cmd/acct
LOCALINC= -I../include
LOCALDEF = $(LARGEFILES)
ACCTDEF_H = $(ACCT_DIR)/acctdef.h
VXACCTDEF_H = vxacctdef.h

INSDIR = $(USRLIB)/acct
OWN = bin
GRP = bin

PROBEFILE = vxdiskusg.c
MAKEFILE = vxdiskusg.mk
BINARIES = vxdiskusg
OBJECTS = vxdiskusg.o

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS); \
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
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) vxdiskusg

clean:
	-rm -f $(OBJECTS)

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

binaries: $(BINARIES)

vxdiskusg: $(OBJECTS) $(LIBDIR)/libvxfs.a
	@if [ -f $(ACCTDEF_H) ]; then \
		$(CC) -I$(ACCT_DIR) $(LDFLAGS) -DACCTDEF_H -o $@ $(OBJECTS) \
			$(SHLIBS) $(LDLIBS); \
	else \
		$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(SHLIBS) $(LDLIBS); \
	fi

vxdiskusg.o: vxdiskusg.c $(VXACCTDEF_H)

