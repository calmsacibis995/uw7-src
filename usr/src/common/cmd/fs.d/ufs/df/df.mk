#	copyright	"%c%"

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/df/df.mk	1.4.5.3"
#ident "$Header$"

include $(CMDRULES)

INSDIR1 = $(USRLIB)/fs/ufs
INSDIR2 = $(ETC)/fs/ufs
OWN = bin
GRP = bin
LDLIBS = -lgen

all:  df

df: df.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ df.o $(OBJS) $(LDLIBS) $(ROOTLIBS)

install: df
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) df
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) df

clean:
	-rm -f df.o

clobber: clean
	rm -f df
