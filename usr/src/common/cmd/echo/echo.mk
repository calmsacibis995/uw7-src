#	copyright	"%c%"

#ident	"@(#)echo:echo.mk	1.9.3.2"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for echo

OWN = bin
GRP = bin

all: echo

echo: echo.o 
	$(CC) -o echo echo.o  $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

echo.o: echo.c

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) echo

clean:
	rm -f echo.o

clobber: clean
	rm -f echo

lintit:
	$(LINT) $(LINTFLAGS) echo.c

# These targets are useful but optional

partslist:
	@echo echo.mk $(SOURCES) $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo echo | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit echo.mk $(LOCALINCS) $(SOURCES) -o $(OBJECTS) echo
