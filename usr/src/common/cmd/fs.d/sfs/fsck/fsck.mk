#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/fsck/fsck.mk	1.5.6.5"
#ident "$Header$"

include $(CMDRULES)
INSDIR1 = $(USRLIB)/fs/sfs
INSDIR2 = $(ETC)/fs/sfs
OWN = bin
GRP = bin
LOCALDEF = -D_KMEMUSER
LDLIBS = -lgen

OBJS=   dir.o inode.o pass1.o pass1b.o pass2.o  Opass2.o \
	pass3.o pass4.o pass5.o setup.o utilities.o \
	sfs_subr.o sfs_tables.o

all: fsck

fsck: $(OBJS) main.o
	$(CC) $(CFLAGS) -o $@ main.o $(OBJS) $(LDLIBS) $(ROOTLIBS)
	$(CC) $(CFLAGS) -o $@.dy main.o $(OBJS) $(LDLIBS) $(SHLIBS)

dir.o:	dir.c \
	fsck.h \
	$(INC)/stdio.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/mntent.h \
	$(INC)/sys/acl.h \
	$(INC)/sys/fs/sfs_fs.h \
	$(INC)/sys/fs/sfs_fsdir.h \
	$(INC)/sys/fs/sfs_inode.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mnttab.h

inode.o:	inode.c \
	fsck.h \
	$(INC)/stdio.h \
	$(INC)/pwd.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/mntent.h \
	$(INC)/sys/acl.h \
	$(INC)/sys/fs/sfs_fs.h \
	$(INC)/sys/fs/sfs_fsdir.h \
	$(INC)/sys/fs/sfs_inode.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mnttab.h

main.o:	main.c \
	fsck.h \
	$(INC)/stdio.h \
	$(INC)/priv.h \
	$(INC)/string.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/mntent.h \
	$(INC)/sys/acl.h \
	$(INC)/sys/fs/sfs_fs.h \
	$(INC)/sys/fs/sfs_fsdir.h \
	$(INC)/sys/fs/sfs_inode.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/wait.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mnttab.h

pass1.o:	pass1.c \
	fsck.h \
	$(INC)/stdio.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/mntent.h \
	$(INC)/sys/acl.h \
	$(INC)/sys/fs/sfs_fs.h \
	$(INC)/sys/fs/sfs_fsdir.h \
	$(INC)/sys/fs/sfs_inode.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mnttab.h

pass1b.o:	pass1b.c \
	fsck.h \
	$(INC)/stdio.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/mntent.h \
	$(INC)/sys/acl.h \
	$(INC)/sys/fs/sfs_fs.h \
	$(INC)/sys/fs/sfs_fsdir.h \
	$(INC)/sys/fs/sfs_inode.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mnttab.h

pass2.o:	pass2.c \
	fsck.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/mntent.h \
	$(INC)/sys/acl.h \
	$(INC)/sys/fs/sfs_fs.h \
	$(INC)/sys/fs/sfs_fsdir.h \
	$(INC)/sys/fs/sfs_inode.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mnttab.h

pass3.o:	pass3.c \
	fsck.h \
	$(INC)/stdio.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/mntent.h \
	$(INC)/sys/acl.h \
	$(INC)/sys/fs/sfs_fs.h \
	$(INC)/sys/fs/sfs_fsdir.h \
	$(INC)/sys/fs/sfs_inode.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mnttab.h

pass4.o:	pass4.c \
	fsck.h \
	$(INC)/stdio.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/mntent.h \
	$(INC)/sys/acl.h \
	$(INC)/sys/fs/sfs_fs.h \
	$(INC)/sys/fs/sfs_fsdir.h \
	$(INC)/sys/fs/sfs_inode.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mnttab.h

pass5.o:	pass5.c \
	fsck.h \
	$(INC)/stdio.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/types.h \
	$(INC)/sys/mntent.h \
	$(INC)/sys/acl.h \
	$(INC)/sys/fs/sfs_fs.h \
	$(INC)/sys/fs/sfs_fsdir.h \
	$(INC)/sys/fs/sfs_inode.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mnttab.h

setup.o:	setup.c \
	fsck.h \
	$(INC)/stdio.h \
	$(INC)/priv.h \
	$(INC)/string.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/types.h \
	$(INC)/sys/mntent.h \
	$(INC)/sys/acl.h \
	$(INC)/sys/fs/sfs_fs.h \
	$(INC)/sys/fs/sfs_fsdir.h \
	$(INC)/sys/fs/sfs_inode.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mnttab.h

sfs_subr.o:    sfs_subr.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/time.h \
	$(INC)/sys/fs/sfs_fs.h \
	$(INC)/sys/fs/sfs_tables.h \
	$(INC)/sys/cmn_err.h  \
	$(INC)/sys/debug.h

sfs_tables.o:    sfs_tables.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/fs/sfs_tables.h 

utilities.o:    utilities.c \
	fsck.h \
	$(INC)/stdio.h \
	$(INC)/priv.h \
	$(INC)/string.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/types.h \
	$(INC)/sys/mntent.h \
	$(INC)/sys/acl.h \
	$(INC)/sys/fs/sfs_fs.h \
	$(INC)/sys/fs/sfs_fsdir.h \
	$(INC)/sys/fs/sfs_inode.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/signal.h  \
	$(INC)/sys/cmn_err.h  \
	$(INC)/sys/mnttab.h

install: fsck
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fsck
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fsck
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fsck.dy
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fsck.dy


clean:     
	-rm -f $(OBJS) main.o
	
clobber: clean
	rm -f fsck fsck.dy
