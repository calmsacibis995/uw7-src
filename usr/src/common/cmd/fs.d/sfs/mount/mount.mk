#	copyright	"%c%"

#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/mount/mount.mk	1.4.5.4"
#ident "$Header$"

include $(CMDRULES)
INSDIR1 = $(USRLIB)/fs/sfs
INSDIR2 = $(ETC)/fs/sfs
OWN = bin
GRP = bin
LDLIBS = -lgen

all:  mount

mount: mount.o
	$(CC) $(LDFLAGS) -o $@ $@.o $(LDLIBS) $(ROOTLIBS)
	$(CC) $(LDFLAGS) -o $@.dy $@.o $(LDLIBS) $(SHLIBS)

mount.o: mount.c \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/stdio.h \
	$(INC)/priv.h \
	$(INC)/mac.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/mntent.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/mnttab.h \
	$(INC)/sys/mount.h \
	$(INC)/sys/wait.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/vfstab.h \
	$(INC)/sys/fsid.h

install: mount
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mount.dy
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mount.dy

clean:
	-rm -f mount.o

clobber: clean
	rm -f mount mount.dy
