#	copyright	"%c%"

#ident	"@(#)who:common/cmd/who/who.mk	1.9.5.7"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for who 

OWN = bin
GRP = bin

MAINS = who who.dy
OBJECTS = who.o Getinittab.o
LDLIBS = -lgen

all: $(MAINS)

who: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

who.dy: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

who.o:  $(INC)/errno.h $(INC)/sys/errno.h \
	 $(INC)/fcntl.h $(INC)/stdio.h \
	 $(INC)/string.h $(INC)/sys/types.h \
	 $(INC)/unistd.h $(INC)/stdlib.h \
	 $(INC)/sys/stat.h \
	 $(INC)/pfmt.h

Getinittab.o:  $(INC)/errno.h $(INC)/sys/errno.h \
	 $(INC)/fcntl.h $(INC)/stdio.h \
	 $(INC)/string.h $(INC)/sys/types.h \
	 $(INC)/unistd.h $(INC)/stdlib.h \
	 $(INC)/sys/stat.h $(INC)/time.h \
	 $(INC)/utmp.h $(INC)/locale.h \
	 $(INC)/pfmt.h

install: all
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) who
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) who.dy
	-/bin/mv $(USRBIN)/who.dy $(USRBIN)/who

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) who.c

# These targets are useful but optional

partslist:
	@echo who.mk who.c c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo who | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit who.mk $(LOCALINCS) who.c -o who.o who
