#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/fsck/fsck.mk	1.5.10.5"
#ident	"$Header$"

include $(CMDRULES)
INSDIR1 = $(USRLIB)/fs/ufs
INSDIR2 = $(ETC)/fs/ufs
OWN = bin
GRP = bin
LOCALDEF = -D_KMEMUSER
LDLIBS = -lgen

OBJS=   dir.o inode.o pass1.o pass1b.o pass2.o Opass2.o \
	pass3.o pass4.o pass5.o setup.o utilities.o
UFSOBJS= ufs_subr.o ufs_tables.o

all: fsck

fsck: $(OBJS) $(UFSOBJS) main.o
	$(CC) $(CFLAGS) -o $@ main.o $(OBJS) $(UFSOBJS) $(LDLIBS) $(ROOTLIBS)
	$(CC) $(CFLAGS) -o $@.dy main.o $(OBJS) $(UFSOBJS) $(LDLIBS) $(SHLIBS)

install: fsck
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fsck
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fsck
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fsck.dy
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fsck.dy

clean:     
	-rm -f $(OBJS) $(UFSOBJS) main.o
	
clobber: clean
	rm -f fsck fsck.dy
