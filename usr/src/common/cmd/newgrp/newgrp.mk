#	copyright	"%c%"

#ident	"@(#)newgrp:newgrp.mk	1.6.10.4"
#ident  "$Header$"

include $(CMDRULES)

#	Makefile for <newgrp>

OWN = root
GRP = sys
LDLIBS = -lgen

all: newgrp

newgrp: newgrp.o 
	$(CC) -o newgrp newgrp.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

newgrp.o: newgrp.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/string.h \
	$(INC)/stdlib.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/sys/secsys.h \
	$(INC)/priv.h

install: all
	 $(INS) -f $(USRBIN) -m 04755 -u $(OWN) -g $(GRP) newgrp

clean:
	rm -f newgrp.o

clobber: clean
	rm -f newgrp

lintit:
	$(LINT) $(LINTFLAGS) newgrp.c

#	These targets are useful but optional

partslist:
	@echo newgrp.mk newgrp.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo newgrp | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit newgrp.mk $(LOCALINCS) newgrp.c -o newgrp.o newgrp
