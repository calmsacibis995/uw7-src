#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)col:col.mk	1.2.4.1"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

FILE = col

all: $(FILE)

$(FILE): $(FILE).o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

install: all
	$(INS) -f $(INSDIR) -m 555 -u $(OWN) -g $(GRP) $(FILE)

clean:
	rm -rf *.o

clobber: clean
	rm -f $(FILE)
