#	copyright	"%c%"

#ident	"@(#)test:i386/cmd/test/test.mk	1.1.2.2"
#ident  "$Header$"

include $(CMDRULES)


OWN = bin
GRP = bin

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved

#	This Module contains Proprietary Information of Microsoft
#	Corporation and should be treated as Confidential.

# Makefile for test.sh

# to install when not privileged
# set $(CH) in the environment to #

all: test

test: test.sh
	cp test.sh test

install: all
	 $(INS) -f $(USRBIN) -m 0775 -u $(OWN) -g $(GRP) test 

clean:

clobber: clean
	rm -f test
