#ident	"@(#)auditset.mk	1.2"
#ident  "$Header$"

include $(CMDRULES)

INSDIR=$(USRSBIN)
AUDITDIR=$(ETC)/security/audit
OWN=root
GRP=audit
SRCDIR=.
LDLIBS=-lia -lgen

MAKEFILE = auditset.mk
SOURCE = auditset.c
OBJECTS = auditset.o
MAINS = auditset classes

all:	$(MAINS)

auditset:	$(OBJECTS)
	$(CC) $(OBJECTS) -o auditset  $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

auditset.o: auditset.c \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/pwd.h \
	$(INC)/string.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h \
	$(INC)/sys/param.h \
	$(INC)/sys/procfs.h \
	$(INC)/audit.h \
	$(INC)/mac.h \
	$(INC)/dirent.h \
	$(INC)/pfmt.h \
	$(INC)/locale.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h \
	$(INC)/ctype.h

classes: class
	grep -v "^#ident" class > classes
#ident  "$Header$"

install:	$(MAINS) $(INSDIR)
	- [ -d $(AUDITDIR) ] || mkdir -p $(AUDITDIR)
	$(INS) -f $(AUDITDIR) -m 0664 -u $(OWN) -g $(GRP) classes
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) auditset

clean:
	-rm -f $(OBJECTS)

clobber: clean
	-rm -f $(MAINS)

strip:
	$(STRIP) $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCE)

remove:
	cd $(INSDIR);	rm -f $(MAINS)

$(INSDIR):
	[ -d $@ ] || mkdir $@ ; chmod 755 $@ ; $(CH)chown bin $@

partslist:
	@echo $(MAKEFILE) $(SRCDIR) $(SOURCE)  |  tr ' ' '\012'  |  sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
		sed -e 's;^;$(INSDIR)/;' -e 's;//*;/;g'

productdir:
	@echo $(INSDIR)

