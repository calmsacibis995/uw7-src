#ident	"@(#)kdb.cmd:i386/cmd/kdb/kdb.mk	1.3"

include	$(CMDRULES)


all:	kdb

kdb: \
	kdb.o \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysi86.h \
	$(INC)/errno.h
	$(CC) $(LDFLAGS) -o $@ $@.o $(ROOTLIBS)

install: all $(SBIN)
	$(INS) -f $(SBIN) kdb

$(SBIN):
	-mkdir -p $@

clean:
	rm -f *.o 

clobber:	clean
	rm -f kdb
