#ident	"@(#)mp.cmds:common/cmd/psrinfo/psrinfo.mk	1.2"

include $(CMDRULES)

OC=$(CFLAGAS)
CFLAGS= ${OC}

OWN = root
GRP = sys

CMD= psrinfo
SOURCES=psrinfo.c

all: $(CMD)

psrinfo: psrinfo.o 
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

psrinfo.o: psrinfo.c 

install: all
	-rm -f $(ETC)/$(CMD)
	-rm -f $(USRSBIN)/$(CMD)
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) $(CMD)
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) $(CMD)
	-$(SYMLINK) /sbin/$(CMD) $(ETC)/$(CMD)

clean:
	rm -f *.o

clobber: clean
	rm -f $(CMD)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
