#	copyright	"%c%"

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/fsdb/fsdb.mk	1.3.5.4"
#ident "$Header$"

include $(CMDRULES)

INSDIR1 = $(USRLIB)/fs/ufs
INSDIR2 = $(ETC)/fs/ufs
OWN = bin
GRP = bin
LDLIBS = -lgen

all:  fsdb

fsdb: fsdb.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(NOSHLIBS) $(LDLIBS)

install: fsdb
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fsdb
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fsdb

clean:
	-rm -f fsdb.o

clobber: clean
	rm -f fsdb
