#	copyright	"%c%"

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/labelit/labelit.mk	1.3.5.4"
#ident "$Header$"

include $(CMDRULES)

INSDIR1 = $(USRLIB)/fs/ufs
INSDIR2 = $(ETC)/fs/ufs
OWN = bin
GRP = bin
LDLIBS = -lgen

all:  labelit

labelit: labelit.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: labelit
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) labelit
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) labelit

clean:
	-rm -f labelit.o

clobber: clean
	rm -f labelit
