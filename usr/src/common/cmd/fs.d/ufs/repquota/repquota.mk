#	copyright	"%c%"

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/repquota/repquota.mk	1.7.7.1"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/ufs
SINSDIR = /usr/lib/fs/ufs
INSDIR2 = $(USRSBIN)
OWN = bin
GRP = bin
OBJS=
LDLIBS = -lgen

all:  repquota

repquota: repquota.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: repquota
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) repquota
	
clean:
	-rm -f repquota.o

clobber: clean
	rm -f repquota
