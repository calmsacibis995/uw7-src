#	copyright	"%c%"

#ident	"@(#)touch:touch.mk	1.12.9.1"
#ident  "$Header$"

include $(CMDRULES)

#	Makefile for touch 

LOCALDEF = -D_FILE_OFFSET_BITS=64

OWN = bin
GRP = bin

LDLIBS = -lgen

all: touch

touch: touch.o 
	$(CC) -o touch touch.o  $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

touch.o: touch.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/pwd.h \
	$(INC)/time.h \
	$(INC)/unistd.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/stdlib.h

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) touch
	-rm -f $(USRBIN)/settime
	ln $(USRBIN)/touch $(USRBIN)/settime

clean:
	rm -f touch.o

clobber: clean
	rm -f touch

lintit:
	$(LINT) $(LINTFLAGS) touch.c

#	These targets are useful but optional

partslist:
	@echo touch.mk touch.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo touch | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit touch.mk $(LOCALINCS) touch.c -o touch.o touch
