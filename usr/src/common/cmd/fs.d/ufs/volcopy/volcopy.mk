#	copyright	"%c%"

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/volcopy/volcopy.mk	1.6.7.2"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/ufs
OWN = bin
GRP = bin
LDLIBS = -lgenIO

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
