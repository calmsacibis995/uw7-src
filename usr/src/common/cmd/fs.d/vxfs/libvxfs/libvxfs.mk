# @(#)src/uw/cmd/vxfs/libvxfs/libvxfs.mk	3.8.9.2 11/21/97 20:28:03 - 
#ident	"@(#)vxfs:src/uw/cmd/vxfs/libvxfs/libvxfs.mk	3.8.9.2 (edited)"
#ident	"@(#)vxfs.cmds:common/cmd/fs.d/vxfs/libvxfs/libvxfs.mk	1.1.6.1"
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
LOCALDEF = $(LARGEFILES)
LOCALINC = -I../include

LIBRARY = libvxfs.a
ILIBRARY = libvxfsi.a
QLIBRARY = libvxquota.a
TARGETS	= $(LIBRARY) $(ILIBRARY) $(QLIBRARY) vxfs.str
QLDLIBS = $(LIBRARY)

OBJECTS	= bmap.o \
	cmdline.o \
	devops.o \
	ilist.o \
	machdep.o \
	mntpt.o \
	path.o \
	rdwri.o

QOBJECTS = machquota.o \
	quota.o

LIBOBJS	= print.o
ILIBOBJS = iprint.o

PROBEFILE = bmap.c
MAKEFILE = libvxfs.mk

.MUTEX:	$(LIBOBJS) $(ILIBOBJS) iprint.c

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(TARGETS) $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
		for fl in $(TARGETS); do \
			if [ ! -r $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi

install: all msginstall

clean:
	rm -f $(OBJECTS) $(LIBOBJS) $(ILIBOBJS) iprint.c
	rm -f $(QOBJECTS)

clobber: clean
	rm -f $(TARGETS)

vxfs.str:	msgs.str
	sed -e "/^\/\*/d" msgs.str >$@

msginstall:
	- [ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 vxfs.str

$(LIBRARY): $(OBJECTS) $(LIBOBJS)
	$(AR) -cr $(LIBRARY) $(OBJECTS) print.o

$(ILIBRARY): $(OBJECTS) $(ILIBOBJS)
	$(AR) -cr $(ILIBRARY) $(OBJECTS) iprint.o

iprint.c:	print.c
	rm -f iprint.c ; cp print.c iprint.c

$(ILIBOBJS):	iprint.c \
	$(FRC)
	$(CC) -DVXI18N $(CFLAGS) $(INCLIST) $(DEFLIST) -c iprint.c

$(LIBOBJS):	print.c \
	$(FRC)
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c print.c

$(QLIBRARY): $(QOBJECTS)
	$(AR) -cr $(QLIBRARY) $(QOBJECTS)
