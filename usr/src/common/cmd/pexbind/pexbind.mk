#ident	"@(#)mp.cmds:common/cmd/pexbind/pexbind.mk	1.1"

include $(CMDRULES)

OWN = root
GRP = sys
CMDS=  pexbind
SOURCES= pexbind.c

all: $(CMDS)

pexbind: pexbind.o
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

install: all
	-rm -f $(ETC)/pexbind
	-rm -f $(USRSBIN)/pexbind
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) pexbind
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) pexbind
	-$(SYMLINK) /sbin/pexbind $(ETC)/pexbind

clean:
	rm -f *.o

clobber: clean
	rm -f $(CMDS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
