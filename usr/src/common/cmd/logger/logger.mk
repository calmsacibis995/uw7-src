#	copyright	"%c%"	*/

#ident	"@(#)logger.mk	1.2"

include $(CMDRULES)

#	Makefile for logger

OWN = bin
GRP = bin

all: logger

logger: logger.o
	$(CC) -o logger logger.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

logger.o: logger.c

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) logger

clean:
	rm -f logger.o

clobber: clean
	rm -f logger

lintit:
	$(LINT) $(LINTFLAGS) logger.c

#	These targets are useful but optional

partslist:
	@echo logger.mk logger.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo logger | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit logger.mk $(LOCALINCS) logger.c -o logger.o logger
