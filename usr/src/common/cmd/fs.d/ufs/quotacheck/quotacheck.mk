#	copyright	"%c%"

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/quotacheck/quotacheck.mk	1.5.6.1"
#ident "$Header$"

include $(CMDRULES)

INCSYS = $(INC)
INSDIR = $(USRLIB)/fs/ufs
INSDIR2 = $(USRSBIN)
OWN = bin
GRP = bin
LDLIBS = -lgen

OBJS=

all:  quotacheck

quotacheck: quotacheck.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: quotacheck
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) quotacheck
	
clean:
	-rm -f quotacheck.o

clobber: clean
	rm -f quotacheck
