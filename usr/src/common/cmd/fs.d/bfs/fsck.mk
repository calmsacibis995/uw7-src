#	copyright	"%c%"

#ident	"@(#)bfs.cmds:common/cmd/fs.d/bfs/fsck.mk	1.11.8.5"
#ident "$Header$"

include $(CMDRULES)

OWN = bin
GRP = bin

FRC =

FILES =\
	fsck.o

all: fsck

fsck: $(FILES)
	$(CC) $(LDFLAGS) -o $@ $(FILES) $(LDLIBS) $(ROOTLIBS)
	$(CC) $(LDFLAGS) -o $@.dy $(FILES) $(LDLIBS) $(SHLIBS)

clean:
	rm -f *.o

install: fsck
	$(INS) -f $(ETC)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) fsck
	$(INS) -f $(USRLIB)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) fsck
	$(INS) -f $(ETC)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) fsck.dy
	$(INS) -f $(USRLIB)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) fsck.dy


clobber: clean
	rm -f fsck fsck.dy

#
# Header dependencies
#

fsck.o: fsck.c \
	$(INC)/stdio.h \
	$(INC)/priv.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fs/bfs.h \
	$(INC)/sys/stat.h \
	$(FRC)
