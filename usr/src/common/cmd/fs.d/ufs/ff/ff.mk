#	copyright	"%c%"

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/ff/ff.mk	1.3.5.3"
#ident "$Header$"

include $(CMDRULES)

INSDIR1 = $(USRLIB)/fs/ufs
OWN = bin
GRP = bin
LDLIBS = -lgen

all:  ff

ff: ff.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: all
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) ff

clean:
	-rm -f ff.o

clobber: clean
	rm -f ff
