#	copyright	"%c%"

#ident	"@(#)tar:tar.mk	1.4.10.1"
#ident  "$Header$"

include $(CMDRULES)

LDLIBS = -lcmd

all: tar

tar: tar.o
	$(CC) -o $@ $@.o $(LDFLAGS) -Kosrcrt $(SHLIBS) $(LDLIBS) 

install: all
	-rm -rf $(ETC)/tar
	$(INS) -f $(USRSBIN) -m 0555 -u bin -g bin tar
	-$(SYMLINK) /usr/sbin/tar $(ETC)/tar
	-mkdir ./tmp
	-$(CP) tar.dfl ./tmp/tar
	$(INS) -f $(ETC)/default -m 0444 -u root -g sys ./tmp/tar
	-rm -rf ./tmp

clean:
	rm -f *.o

clobber: clean
	rm -f tar

lintit:
	$(LINT) $(LINTFLAGS) tar.c
