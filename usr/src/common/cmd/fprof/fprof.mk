#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fprof:fprof.mk	1.1"

include $(CMDRULES)

CMDBASE		= ..
SGSBASE		= ../sgs

LDFLAGS		= -s

ENVPARMS	= \
	CMDRULES="$(CMDRULES)" CMDBASE="$(CMDBASE)" \
	SGSBASE="$(SGSBASE)" LDFLAGS="$(LDFLAGS)" LIBS="$(LIBS)"

all:
	cd $(CPU) ; $(MAKE) all $(ENVPARMS)

install:
	cd $(CPU) ; $(MAKE) install

lintit:
	cd $(CPU) ; $(MAKE) lintit $(ENVPARMS)

clean:
	cd $(CPU) ; $(MAKE) clean

clobber:
	cd $(CPU) ; $(MAKE) clobber
