#	copyright	"%c%"

#ident	"@(#)pathchk:pathchk.mk	1.1.2.1"

include $(CMDRULES)

#	Makefile for pathchk

LOCALDEF = -D_FILE_SIZE_BITS=64

OWN = bin
GRP = bin

all: pathchk

pathchk: pathchk.o
	$(CC) -o pathchk pathchk.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

pathchk.o: pathchk.c

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) pathchk

clean:
	rm -f pathchk.o

clobber: clean
	rm -f pathchk

lintit:
	$(LINT) $(LINTFLAGS) pathchk.c

#	These targets are useful but optional

partslist:
	@echo pathchk.mk pathchk.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo pathchk | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit pathchk.mk $(LOCALINCS) pathchk.c -o pathchk.o pathchk
