#	copyright	"%c%"

#ident	"@(#)bfs.cmds:common/cmd/fs.d/bfs/mkfs.mk	1.7.7.3"
#ident "$Header$"

include $(CMDRULES)

INCSYS = $(INC)
OWN = bin
GRP = bin
FRC =

FILES =\
	mkfs.o

all: mkfs

mkfs: $(FILES)
	$(CC) $(LDFLAGS) -o mkfs $(FILES) $(LDLIBS) $(ROOTLIBS)
	$(CC) $(LDFLAGS) -o mkfs.dy $(FILES) $(LDLIBS) $(SHLIBS)

install: mkfs
	$(INS) -f $(ETC)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) mkfs
	$(INS) -f $(USRLIB)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) mkfs
	$(INS) -f $(ETC)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) mkfs.dy
	$(INS) -f $(USRLIB)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) mkfs.dy

clean:
	rm -f *.o

clobber: clean
	rm -f mkfs mkfs.dy *.o
#
# Header dependencies
#

mkfs.o: mkfs.c \
	$(INC)/stdio.h \
	$(INCSYS)/sys/types.h \
	$(INCSYS)/sys/vnode.h \
	$(INCSYS)/sys/fs/bfs.h \
	$(INCSYS)/sys/vtoc.h \
	$(INCSYS)/sys/stat.h \
	$(INCSYS)/sys/fcntl.h \
	$(FRC)
