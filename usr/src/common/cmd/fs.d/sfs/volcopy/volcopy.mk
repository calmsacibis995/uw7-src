#	copyright	"%c%"

#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/volcopy/volcopy.mk	1.1.2.2"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/sfs
OWN = bin
GRP = bin

LDLIBS=-lgenIO
OBJS=

all:  volcopy

volcopy: volcopy.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(ROOTLIBS)

install: volcopy
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) volcopy

clean:
	-rm -f volcopy.o

clobber: clean
	rm -f volcopy
