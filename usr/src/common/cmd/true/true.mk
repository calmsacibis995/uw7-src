#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)true:common/cmd/true/true.mk	1.4.7.2"
#ident "$Header$"

include $(CMDRULES)

OWN = root
GRP = sys

all: install

install:
	cp true.sh true
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) true

clean:
	rm -f true

clobber: clean
	rm -f true
