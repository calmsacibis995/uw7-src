#	copyright	"%c%"

#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/tunefs/tunefs.mk	1.2.1.3"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/sfs
INSDIR2 = $(ETC)/fs/sfs
OWN = bin
GRP = bin
LDLIBS = -lgen

OBJS=

all:  tunefs

tunefs: tunefs.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(ROOTLIBS)

install: tunefs
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) tunefs
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) tunefs
	
clean:
	-rm -f tunefs.o

clobber: clean
	rm -f tunefs
