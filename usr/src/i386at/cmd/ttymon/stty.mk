#ident	"@(#)stty.mk	1.3"

include $(CMDRULES)

#	Makefile for stty 

OWN = root
GRP = sys

OBJECTS = stty.o sttytable.o sttyparse.o
SOURCES = $(OBJECTS:.o=.c)

LOCALDEF = -DMERGE386

all: stty

stty: $(OBJECTS)
	$(CC) -o stty $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

stty.o: stty.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/locale.h \
	$(INC)/sys/types.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/sys/stermio.h \
	$(INC)/sys/termiox.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	stty.h

sttytable.o: sttytable.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/sys/stermio.h \
	$(INC)/sys/termiox.h \
	stty.h

sttyparse.o: sttyparse.c \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/sys/types.h \
	$(INC)/ctype.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/stermio.h \
	$(INC)/sys/termiox.h \
	stty.h \
	$(INC)/pfmt.h \
	$(INC)/sys/ioctl.h

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f stty

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) stty

#	These targets are useful but optional

partslist:
	@echo stty.mk $(SOURCES) $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo stty | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit stty.mk $(LOCALINCS) $(SOURCES) -o $(OBJECTS) stty
