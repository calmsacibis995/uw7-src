#	copyright	"%c%"

#ident	"@(#)sum:sum.mk	1.4.7.1"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for sum

LOCALDEF = -D_FILE_OFFSET_BITS=64

OWN = bin
GRP = bin

all: sum

sum: sum.c \
	$(INC)/stdio.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) sum

clean:
	rm -f sum.o

clobber: clean
	rm -f sum

lintit:
	$(LINT) $(LINTFLAGS) sum.c

