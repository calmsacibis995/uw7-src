#ident	"@(#)psradm:common/cmd/psradm/psradm.mk	1.2"

include $(CMDRULES)

OWN = root
GRP = sys

CMDS= psradm

LDLIBS = -lgen

all: $(CMDS)

psradm: psradm.o
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

psradm.o: psradm.c 

install: all
	-rm -f $(ETC)/psradm
	-rm -f $(USRSBIN)/psradm
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) psradm
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) psradm
	-$(SYMLINK) /sbin/psradm $(ETC)/psradm

clean:
	rm -f *.o

clobber: clean
	rm -f psradm

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
