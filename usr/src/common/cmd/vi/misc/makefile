#	copyright	"%c%"

#ident	"@(#)vi:misc/makefile	1.18.1.3"
#ident  "$Header$"

include $(CMDRULES)

#	Makefile for misc directory for vi

OWN = bin
GRP = bin

MAINS = ctags fold cxref

OBJECTS = ctags.o fold.o 

SOURCES = ctags.c fold.c cxref.sh

all: $(MAINS)

ctags: ctags.o 
	$(CC) -o ctags ctags.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

fold: fold.o 
	$(CC) -o fold fold.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ctags.o: ctags.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h 

fold.o:  fold.c \
	$(INC)/stdio.h 

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ctags 

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

#	These targets are useful but optional

partslist:
	@echo makefile $(SOURCES) $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo ctags | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit makefile $(LOCALINCS) $(SOURCES) READ_ME del.h.cb -o $(OBJECTS) $(MAINS)
