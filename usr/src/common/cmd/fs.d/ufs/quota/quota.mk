#	copyright	"%c%"

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/quota/quota.mk	1.7.6.3"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/ufs
SINSDIR = /usr/lib/fs/ufs
INSDIR2 = $(USRSBIN)
OWN = bin
GRP = bin
LDLIBS = -lgen

OBJS=

all:  quota

quota: quota.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: quota
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) quota;
	-rm -f $(INSDIR2)/quota $(USRBIN)/quota
	
clean:
	-rm -f quota.o

clobber: clean
	rm -f quota
