#	copyright	"%c%"

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mkdir:mkdir.mk	1.11.11.1"

include $(CMDRULES)

#	mkdir make file

OWN = bin
GRP = bin

LDLIBS = -lgen -lcmd

all: mkdir

mkdir: mkdir.o
	$(CC) -o mkdir mkdir.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

mkdir.o: mkdir.c \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h \
	$(INC)/mac.h $(INC)/sys/mac.h \
	$(INC)/deflt.h \
	$(INC)/priv.h $(INC)/sys/privilege.h \
	$(INC)/stdlib.h \
	$(INC)/pwd.h \
	$(INC)/unistd.h $(INC)/sys/unistd.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h	\
	$(INC)/ctype.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) mkdir

clean:
	-rm -f mkdir.o

clobber: clean
	rm -f mkdir
