#	copyright	"%c%"

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/tunefs/tunefs.mk	1.5.6.3"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/ufs
INSDIR2 = $(ETC)/fs/ufs
OWN = bin
GRP = bin
LDLIBS = -lgen

OBJS=

all:  tunefs

tunefs: tunefs.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: tunefs
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) tunefs
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) tunefs
	
clean:
	-rm -f tunefs.o

clobber: clean
	rm -f tunefs
