#	copyright	"%c%"

#ident	"@(#)cpiopc:cpiopc.mk	1.3.2.5"
#ident  "$Header$"

include $(CMDRULES)

#	Makefile for cpiopc

OWN = root
GRP = sys
LDLIBS = -lgen

LOCALDEF = -D_STYPES

all: .cpiopc

.cpiopc: cpiopc.o 
	$(CC) -o .cpiopc cpiopc.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

cpiopc.o: cpiopc.c \
	$(INC)/stdio.h \
	$(INC)/signal.h \
	$(INC)/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/pwd.h 

install: all
	 $(INS) -f $(USRSBIN) -m 0755 -u $(OWN) -g $(GRP) .cpiopc

clean:
	rm -f cpiopc.o

clobber: clean
	rm -f .cpiopc

lintit:
	$(LINT) $(LINTFLAGS) cpiopc.c

#	These targets are useful but optional

partslist:
	@echo Makefile cpiopc.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRSBIN) | tr ' ' '\012' | sort

product:
	@echo .cpiopc | tr ' ' '\012' | \
	sed 's;^;$(USRSBIN)/;'

srcaudit:
	@fileaudit Makefile $(GLOBALINCS) cpiopc.c -o cpiopc.o .cpiopc
