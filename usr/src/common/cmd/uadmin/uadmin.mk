#	copyright	"%c%"

#ident	"@(#)uadmin:uadmin.mk	1.10.3.3"

include $(CMDRULES)

OWN = root
GRP = sys

MSGS = uadmin.str

all: uadmin

uadmin: uadmin.c \
	$(INC)/stdio.h \
	$(INC)/signal.h \
	$(INC)/sys/signal.h \
	$(INC)/priv.h \
	$(INC)/sys/uadmin.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@.dy $@.c $(LDFLAGS) $(LDLIBS)

install: all $(MSGS)
	-rm -f $(ETC)/uadmin
	-rm -f $(ETC)/uadmin.dy
	-rm -f $(USRSBIN)/uadmin
	-rm -f $(USRSBIN)/uadmin.dy
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) uadmin
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) uadmin
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) uadmin.dy
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) uadmin.dy
	-$(SYMLINK) /sbin/uadmin $(ETC)/uadmin
	-$(SYMLINK) /sbin/uadmin.dy $(ETC)/uadmin.dy
	-[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 uadmin.str

clean:
	rm -f *.o

clobber: clean
	rm -f uadmin uadmin.dy

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
