#	copyright	"%c%"

#ident	"@(#)strings:common/cmd/strings/strings.mk	1.5.5.3"
#ident "$Header$"

include $(CMDRULES)

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved
#	This Module contains Proprietary Information of Microsoft
#	Corporation and should be treated as Confidential.
# Makefile for strings

OWN = bin
GRP = bin

LDLIBS = $(LIBELF)

all: strings

strings: strings.o
	$(CC) -o strings strings.o $(LDFLAGS) $(LDLIBS)

strings.o: strings.c \
	$(INC)/stdio.h \
	x.out.h \
	$(INC)/ctype.h \
	$(INC)/libelf.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) strings

clean:
	rm -f strings.o

clobber: clean
	rm -f strings

lintit:
	$(LINT) $(LINTFLAGS) strings.c

