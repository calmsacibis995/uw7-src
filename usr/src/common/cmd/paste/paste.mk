#	copyright	"%c%"

#ident	"@(#)paste:paste.mk	1.2.5.2"

include $(CMDRULES)

#	Makefile for paste

OWN = bin
GRP = bin

LDLIBS = -lw

all: paste

paste: paste.o
	$(CC) -o paste paste.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

paste.o: paste.c \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/sys/euc.h \
	$(INC)/getwidth.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/limits.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) paste

clean:
	rm -f paste.o

clobber: clean
	rm -f paste

lintit:
	$(LINT) $(LINTFLAGS) paste.c

