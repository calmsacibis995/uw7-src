#ident	"@(#)tr:tr.mk	1.3.7.1"

include $(CMDRULES)

OWN = bin
GRP = bin

LDLIBS = -lw

OBJECTS = tr.o mtr.o

SOURCES = tr.c mtr.c

all: tr

tr: $(OBJECTS)
	$(CC) -o tr $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

tr.o: tr.c tr.h \
	$(INC)/stdio.h \
	$(INC)/sys/euc.h \
	$(INC)/getwidth.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h

mtr.o: mtr.c tr.h \
	$(INC)/stdio.h \
	$(INC)/sys/euc.h \
	$(INC)/pfmt.h

install: all
	$(INS) -f $(USRBIN) -m 555 -u $(OWN) -g $(GRP) tr

clean:
	rm -f $(OBJECTS)
	
clobber: clean
	rm -f tr

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

# 	These targets are useful but optional

remove:
	cd $(USRBIN); rm -f tr

partslist:
	@echo tr.mk $(LOCALINCS) $(SOURCES) | tr ' ' '\012' | sort

product:
	@echo tr | tr ' ' '\012' | \
	sed -e 's;^;$(USRBIN)/;' -e 's;//*;/;g'

productdir:
	@echo $(USRBIN)
