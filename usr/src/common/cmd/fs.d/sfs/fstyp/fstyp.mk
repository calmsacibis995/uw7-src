#	copyright	"%c%"

#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/fstyp/fstyp.mk	1.2.3.3"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/sfs
INSDIR2 = $(ETC)/fs/sfs
OWN = bin
GRP = bin

OBJS=

all:  fstyp

fstyp: fstyp.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(NOSHLIBS) $(LDLIBS)

fstyp.o: fstyp.c \
	$(INC)/stdio.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/mntent.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fs/sfs_fs.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/mnttab.h

install: fstyp
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) fstyp
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fstyp

clean:
	-rm -f fstyp.o

clobber: clean
	rm -f fstyp
