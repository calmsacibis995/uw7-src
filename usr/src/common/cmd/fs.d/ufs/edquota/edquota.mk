#	copyright	"%c%"

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/edquota/edquota.mk	1.7.7.1"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/ufs
INSDIR2 = $(USRSBIN)
OWN = bin
GRP = bin
OBJS=
LDLIBS = -lgen

all:  edquota

edquota: edquota.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) edquota
	
clean:
	-rm -f edquota.o

clobber: clean
	rm -f edquota
