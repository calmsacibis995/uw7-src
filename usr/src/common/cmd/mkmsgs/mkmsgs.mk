#	copyright	"%c%"

#ident	"@(#)mkmsgs:mkmsgs.mk	1.6.8.3"

include $(CMDRULES)

#	Makefile for mkmsgs

OWN = root
GRP = root

LDLIBS = -lgen

all: mkmsgs

mkmsgs: mkmsgs.o 
	$(CC) -o mkmsgs mkmsgs.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

mkmsgs.o: mkmsgs.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/priv.h \
	$(INC)/mac.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/unistd.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) mkmsgs

clean:
	rm -f mkmsgs.o

clobber: clean
	rm -f mkmsgs

lintit:
	$(LINT) $(LINTFLAGS) mkmsgs.c

#	These targets are useful but optional

partslist:
	@echo mkmsgs.mk mkmsgs.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo mkmsgs | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit mkmsgs.mk $(LOCALINCS) mkmsgs.c -o mkmsgs.o mkmsgs
