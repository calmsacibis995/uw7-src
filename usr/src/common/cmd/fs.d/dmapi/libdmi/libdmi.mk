# @(#)src/uw/cmd/dmapi/libdmi/libdmi.mk	3.2.9.1 11/28/97 14:28:19 - 
#ident	"@(#)vxfs:src/uw/cmd/dmapi/libdmi/libdmi.mk	3.2.9.1"
#ident	"@(#)vxfs.cmds:common/cmd/fs.d/dmapi/libdmi/libdmi.mk	1.1.1.1"
#
# Copyright (c) 1997 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
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

LDLIBS = -lgen
LOCALDEF = $(LARGEFILES) -DSTATIC=static -DVXI18N
LOCALINC = -I../../vxfs/include -I.

LIBRARY = libdmi.a
TARGETS	= $(LIBRARY)

OBJECTS	= libdmi.o

INC_INSDIR = $(ROOT)/$(MACH)/usr/include

PROBEFILE = libdmi.c
MAKEFILE = libdmi.mk

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(TARGETS) $(MAKEARGS) ;\
	else \
		for fl in $(TARGETS); do \
			if [ ! -f $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi

install: all
	[ -d $(INC_INSDIR) ] || mkdir -p $(INC_INSDIR)
	[ -d $(USRLIB) ] || mkdir -p $(USRLIB)
	$(INS) -f $(INC_INSDIR) -m $(INCMODE) -u $(OWN) -g $(GRP) dmapi.h
	$(INS) -f $(USRLIB) -m 0555 -u $(OWN) -g $(GRP) $(LIBRARY)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(TARGETS)

$(LIBRARY): $(OBJECTS) $(LIBOBJS)
	$(AR) -cr $(LIBRARY) $(OBJECTS)
