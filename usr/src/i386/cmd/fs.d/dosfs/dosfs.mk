#	copyright	"%c%"

#ident	"@(#)dosfs.cmds:dosfs.mk	1.3"
#ident "$Header$"

include $(CMDRULES)

INSDIR1 = $(USRLIB)/fs/dosfs
INSDIR2 = $(ETC)/fs/dosfs
OWN = bin
GRP = bin
LOCALDEF = -D_KMEMUSER
LDLIBS = -lgen

LIBELF =
#
#	Libelf is required for mkfs if ELF_BOOT is defined.  This would allow
#	mkfs to open, parse, and load ELF boot strap code to the disk.  That
#	is, if the name of an ELF boot file was given as the first line of
#	a proto file.
#
# LIBELF = $(LIBELF)

LINTFLAGS = $(DEFLIST)

THISFILE= dosfs.mk

all:	mount fstyp fsck

clean:
	rm -f *.o

clobber: clean clobber_mount clobber_fstyp clobber_fsck

install: install_mount install_fstyp install_fsck

#
#  This is to build dosfs specific mount command
#

mount:	mount.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)
	$(CC) -o $@.dy $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

mount.o:mount.c\
	$(INC)/stdio.h\
	$(INC)/string.h\
	$(INC)/stdlib.h\
	$(INC)/sys/signal.h\
	$(INC)/unistd.h \
	$(INC)/limits.h \
	$(INC)/fcntl.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/time.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/mnttab.h \
	$(INC)/sys/mount.h \
	$(INC)/sys/types.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/fs/bpb.h \
	$(INC)/sys/fs/dosfs_filsys.h

install_mount: mount $(INSDIR1) $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mount.dy
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mount.dy

clobber_mount:	clean
	rm -f mount mount.dy

#
#  This is to build dosfs specific fstyp command
#

fstyp:	fstyp.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)
	$(CC) -o $@.dy $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

fstyp.o:fstyp.c\
	$(INC)/errno.h\
	$(INC)/fcntl.h\
	$(INC)/stdio.h\
	$(INC)/unistd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/fs/bootsect.h

install_fstyp: fstyp $(INSDIR1) $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fstyp
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fstyp
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fstyp.dy
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fstyp.dy

clobber_fstyp:	clean
	rm -f fstyp fstyp.dy

#
#  This is to build dosfs specific fsck command
#

fsck: 	fsck.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)
	$(CC) -o $@.dy $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install_fsck: fsck $(INSDIR1) $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fsck
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fsck
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fsck.dy
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fsck.dy

clobber_fsck: clean
	rm -f fsck fsck.dy

lint_fsck:
	$(LINT) $(LINTFLAGS) fsck.c

#
# Header dependencies
#

fsck.o: fsck.c \
	fsck.h \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/string.h \
	$(INC)/pfmt.h \
	$(INC)/locale.h \
	$(INC)/ctype.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/fs/bootsect.h \
	$(INC)/sys/fs/bpb.h \
	$(INC)/sys/fs/direntry.h

$(INSDIR1):
	mkdir -p $(INSDIR1)

$(INSDIR2):
	mkdir -p $(INSDIR2)
