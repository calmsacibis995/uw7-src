#	copyright	"%c%"

#ident	"@(#)tee:tee.mk	1.6.4.2"

include $(CMDRULES)

#	Makefile for tee 

OWN = bin
GRP = bin

all: tee

tee: tee.o 
	$(CC) -o tee tee.o  $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

tee.o: tee.c \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/fcntl.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/stdlib.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) tee

clean:
	rm -f tee.o

clobber: clean
	rm -f tee

lintit:
	$(LINT) $(LINTFLAGS) tee.c

#	These targets are useful but optional

partslist:
	@echo tee.mk tee.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo tee | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit tee.mk $(LOCALINCS) tee.c -o tee.o tee
