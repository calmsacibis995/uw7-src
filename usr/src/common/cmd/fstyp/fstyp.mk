#ident	"@(#)fstyp:common/cmd/fstyp/fstyp.mk	1.3.11.2"
#ident	"$Header$"

include $(CMDRULES)

INSDIR = $(USRSBIN)
DIRSV = $(USRLIB)/fs/s5
DIRBFS = $(USRLIB)/fs/bfs
OWN = root
GRP = sys

OBJECTS =  S5fstyp.o Bfsfstyp.o

all: fstyp S5fstyp Bfsfstyp

S5fstyp:  S5fstyp.o bsize.o
	$(CC) -o $@  S5fstyp.o bsize.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

Bfsfstyp:   Bfsfstyp.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

fstyp: fstyp.sh

S5fstyp.o:	$(INC)/sys/param.h $(INC)/sys/stat.h \
		$(INC)/time.h \
		$(INC)/sys/types.h  \
		$(INC)/sys/vnode.h $(INC)/sys/fs/s5param.h \
		$(INC)/sys/fs/s5ino.h $(INC)/sys/fs/s5inode.h \
		$(INC)/sys/fs/s5dir.h $(INC)/stdio.h \
		$(INC)/setjmp.h $(INC)/sys/fs/s5filsys.h \
		$(INC)/sys/fcntl.h

bsize.o:  bsize.c \
	$(INC)/sys/types.h \
	$(INC)/sys/fs/s5ino.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/fs/s5filsys.h \
	$(INC)/sys/fs/s5dir.h

clean:
	rm -rf $(OBJECTS) tmp

clobber: clean
	rm -f fstyp S5fstyp Bfsfstyp 

install: all dir
	rm -f $(ETC)/fstyp
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) fstyp
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) fstyp
	$(SYMLINK) /sbin/fstyp $(ETC)/fstyp
	rm -rf ./tmp
	-mkdir ./tmp
	ln S5fstyp ./tmp/fstyp
	$(INS) -f $(DIRSV) -m 0555 -u $(OWN) -g $(GRP) ./tmp/fstyp
	$(INS) -f $(ETC)/fs/s5 -m 0555 -u $(OWN) -g $(GRP) ./tmp/fstyp
	rm -f ./tmp/fstyp
	ln Bfsfstyp ./tmp/fstyp
	$(INS) -f $(DIRBFS) -m 0555 -u $(OWN) -g $(GRP) ./tmp/fstyp
	$(INS) -f $(ETC)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) ./tmp/fstyp
	rm -rf ./tmp

dir:
	[ -d $(DIRSV) ] || mkdir -p $(DIRSV)
	[ -d $(DIRBFS) ] || mkdir -p $(DIRBFS)

strip: all
	$(STRIP) S5fstyp Bfsfstyp 
