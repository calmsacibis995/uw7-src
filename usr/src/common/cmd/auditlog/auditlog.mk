#	copyright	"%c%"

#ident	"@(#)auditlog.mk	1.2"
#ident  "$Header$"

include $(CMDRULES)

OWN=root
GRP=audit

INSDIR=$(USRSBIN)
LDLIBS=-lia -lcmd

MAKEFILE = auditlog.mk
SOURCE = auditlog.c
OBJECTS = auditlog.o
MAINS = auditlog

all:	$(MAINS)

auditlog:	$(OBJECTS)
	$(CC) $(OBJECTS) -o $(MAINS) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

auditlog.o: auditlog.c \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/string.h \
	$(INC)/deflt.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/param.h \
	$(INC)/audit.h \
	$(INC)/mac.h \
	$(INC)/pfmt.h \
	$(INC)/locale.h \
	$(INC)/stdlib.h

install:	$(MAINS) $(INSDIR)
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) $(MAINS)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCE)

strip:
	$(STRIP) $(MAINS)

remove:
	cd $(INSDIR);	rm -f $(MAINS)

$(INSDIR):
	mkdir $(INSDIR);  $(CH)chmod 755 $(INSDIR);  $(CH)chown bin $(INSDIR)

partslist:
	@echo $(MAKEFILE) $(LOCALINC) $(SOURCE)  |  tr ' ' '\012'  |  sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
		sed -e 's;^;$(INSDIR)/;' -e 's;//*;/;g'

productdir:
	@echo $(INSDIR)
