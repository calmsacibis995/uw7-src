#	copyright	"%c%"

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/quotaon/quotaon.mk	1.7.6.1"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/ufs
INSDIR2 = $(USRSBIN)
OWN =  bin
GRP = bin
OBJS=
LDLIBS = -lgen

all:  quotaon

quotaon: quotaon.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@  $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: quotaon
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	rm -f $(INSDIR)/quotaon
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) quotaon
	-rm -f $(INSDIR)/quotaoff
	ln $(INSDIR)/quotaon $(INSDIR)/quotaoff
	
clean:
	-rm -f quotaon.o

clobber: clean
	rm -f quotaon
