#	copyright	"%c%"

#	Portions Copyright (c) 1988, Sun Microsystems, Inc.
#	All Rights Reserved.

#ident	"@(#)split:split.mk	1.3.7.3"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for split

OWN = bin
GRP = bin

LOCAL_LDLIBS = $(LDLIBS) -lgen

all: split

split: split.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/statvfs.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/errno.h $(INC)/sys/errno.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LOCAL_LDLIBS) $(SHLIBS)

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) split

clean:
	rm -f split.o

clobber: clean
	rm -f split

lintit:
	$(LINT) $(LINTFLAGS) split.c

