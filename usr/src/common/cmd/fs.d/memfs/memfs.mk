#ident	"@(#)memfs.cmds:common/cmd/fs.d/memfs/memfs.mk	1.1.1.1"

include $(CMDRULES)

INSDIR1 = $(USRLIB)/fs/memfs
INSDIR2 = $(ETC)/fs/memfs
OWN = bin
GRP = bin


LIBELF =
#
#	Libelf is required for mkfs if ELF_BOOT is defined.  This would allow
#	mkfs to open, parse, and load ELF boot strap code to the disk.  That
#	is, if the name of an ELF boot file was given as the first line of
#	a proto file.
#
# LIBELF = $(LIBELF)

LINTFLAGS = $(DEFLIST)

THISFILE= memfs.mk

all:	mount

clean:
	rm -f *.o

clobber: clean		clobber_mount

install: install_mount

#
#  This is to build memfs specific mount command
#

mount:	mount.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)
	$(CC) -o $@.dy $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

mount.o:mount.c\
	$(INC)/stdio.h\
	$(INC)/sys/signal.h\
	$(INC)/unistd.h\
	$(INC)/sys/errno.h\
	$(INC)/sys/mnttab.h\
	$(INC)/sys/mount.h\
	$(INC)/sys/types.h\
	$(INC)/sys/statvfs.h

install_mount: mount $(INSDIR1) $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mount.dy
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mount.dy

clobber_mount:	clean
	rm -f mount mount.dy

$(INSDIR1):
	mkdir -p $(INSDIR1)

$(INSDIR2):
	mkdir -p $(INSDIR2)

