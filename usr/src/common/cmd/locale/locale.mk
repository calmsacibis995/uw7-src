#ident	"@(#)locale:locale.mk	1.1"

include $(CMDRULES)

#	Makefile for locale 

OWN = bin
GRP = bin

all: locale

locale: locale.o 
	$(CC) -o locale locale.o  $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

locale.o: \
	locale.c \
	$(INC)/unistd.h \
	$(INC)/stdarg.h \
	$(INC)/stdlib.h \
	$(INC)/limits.h \
	$(INC)/locale.h \
	$(INC)/stdio.h \
	$(INC)/pfmt.h \
	$(INC)/langinfo.h 

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) locale

clean:
	rm -f locale.o

clobber: clean
	rm -f locale

lintit:
	$(LINT) $(LINTFLAGS) locale.c

#	These targets are useful but optional

partslist:
	@echo locale.mk locale.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo locale | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit locale.mk $(LOCALINCS) locale.c -o locale.o locale
