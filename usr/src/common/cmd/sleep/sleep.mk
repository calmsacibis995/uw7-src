#	copyright	"%c%"

#ident	"@(#)sleep:sleep.mk	1.5.6.4"

include $(CMDRULES)

#	Makefile for sleep 

OWN = bin
GRP = bin

all: sleep

sleep: sleep.o 
	$(CC) -o sleep sleep.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

sleep.o: sleep.c \
	$(INC)/stdio.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h	\
	$(INC)/errno.h	\
	$(INC)/stdlib.h	\
	$(INC)/unistd.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) sleep

clean:
	rm -f sleep.o

clobber: clean
	rm -f sleep

lintit:
	$(LINT) $(LINTFLAGS) sleep.c

#	These targets are useful but optional

partslist:
	@echo sleep.mk sleep.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo sleep | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit sleep.mk $(LOCALINCS) sleep.c -o sleep.o sleep
