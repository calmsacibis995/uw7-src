#	copyright	"%c%"

#ident	"@(#)auditoff.mk	1.2"
#ident "$Header$"

include $(CMDRULES)

INSDIR=$(USRSBIN)
OWN=root
GRP=audit
SRCDIR=.
LDLIBS=-lia

MAKEFILE = auditoff.mk
SOURCE = auditoff.c
OBJECTS = auditoff.o
MAINS = auditoff

all:	$(MAINS)

auditoff:	$(OBJECTS)
	$(CC) $(OBJECTS) -o $(MAINS) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

auditoff.o: auditoff.c \
	$(INC)/errno.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/sys/time.h \
	$(INC)/audit.h \
	$(INC)/mac.h \
	$(INC)/pfmt.h \
	$(INC)/locale.h \
	$(INC)/stdlib.h

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

install:	$(MAINS) $(INSDIR)
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) $(MAINS)

strip:
	$(STRIP) $(MAINS)

lintit:
	$(LINT) $(CFLAGS) $(SOURCE)

remove:
	cd $(INSDIR);	rm -f $(MAINS)

$(INSDIR):
	mkdir $(INSDIR);  $(CH)chmod 755 $(INSDIR);  $(CH)chown bin $(INSDIR)

partslist:
	@echo $(MAKEFILE) $(SRCDIR) $(SOURCE)  |  tr ' ' '\012'  |  sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
		sed -e 's;^;$(INSDIR)/;' -e 's;//*;/;g'

productdir:
	@echo $(INSDIR)
