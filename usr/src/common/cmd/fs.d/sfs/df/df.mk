#	copyright	"%c%"

#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/df/df.mk	1.2.4.3"
#ident "$Header$"

include $(CMDRULES)

INSDIR1 = $(USRLIB)/fs/sfs
INSDIR2 = $(ETC)/fs/sfs
OWN = bin
GRP = bin
LDLIBS = -lgen

all:  df

df: df.o $(OBJS)
	$(CC) $(LDFLAGS) -o df df.o $(OBJS) $(LDLIBS) $(ROOTLIBS)

df.o:	df.c $(INC)/stdio.h \
	$(INC)/priv.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/mntent.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/acl.h \
	$(INC)/sys/fs/sfs_fs.h \
	$(INC)/sys/fs/sfs_inode.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/mnttab.h

install: df
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) df
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) df

clean:
	-rm -f df.o

clobber: clean
	rm -f df
