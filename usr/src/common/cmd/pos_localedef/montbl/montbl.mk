#	copyright	"%c%"

#ident	"@(#)pos_localedef:common/cmd/pos_localedef/montbl/montbl.mk	1.1.7.3"

include $(CMDRULES)

#	Makefile for montbl

OWN = bin
GRP = bin

all: montbl

montbl: montbl.o 
	$(CC) -o montbl montbl.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

montbl.o: montbl.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/locale.h \
	$(INC)/limits.h

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) montbl ;\
	$(INS) -f $(USRLIB)/locale/C -m 0555 -u $(OWN) -g $(GRP) montbl_C ;\
	$(CH)./montbl montbl_C ;\
	$(CH)$(INS) -f $(USRLIB)/locale/C LC_MONETARY ;\
	$(CH)rm -f LC_MONETARY
	$(INS) -f $(USRLIB)/locale/POSIX -m 0555 -u $(OWN) -g $(GRP) montbl_POSIX ;\
	$(CH)./montbl montbl_POSIX ;\
	$(CH)$(INS) -f $(USRLIB)/locale/POSIX LC_MONETARY ;\
	$(CH)rm -f LC_MONETARY

clean:
	rm -f montbl.o

clobber: clean
	rm -f montbl

lintit:
	$(LINT) $(LINTFLAGS) montbl.c

#	These targets are useful but optional

partslist:
	@echo montbl.mk montbl.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo montbl | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit montbl.mk $(LOCALINCS) montbl.c -o montbl.o montbl
