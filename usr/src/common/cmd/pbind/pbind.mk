#ident	"@(#)mp.cmds:common/cmd/pbind/pbind.mk	1.1"

include $(CMDRULES)

OWN = root
GRP = sys
CMDS= pbind 
SOURCES= pbind.c 

all: $(CMDS)

pbind: pbind.o
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

install: all
	-rm -f $(ETC)/pbind
	-rm -f $(USRSBIN)/pbind
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) pbind
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) pbind
	-$(SYMLINK) /sbin/pbind $(ETC)/pbind

clean:
	rm -f *.o

clobber: clean
	rm -f $(CMDS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
