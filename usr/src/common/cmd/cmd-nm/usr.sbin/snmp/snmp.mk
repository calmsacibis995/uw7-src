#ident	"@(#)snmp.mk	1.3"
#ident "$Header$"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.

# Copyrighted as an unpublished work.
# (c) Copyright 1989 INTERACTIVE Systems Corporation
# All rights reserved.
#

#
# Copyright (c) 1987, 1988, 1989 Kenneth W. Key and Jeffrey D. Case
#
#
# Revision History:
#  6/3/89  JDC
#  added cleanup of bin directory on make clean
#

include ${CMDRULES}

# XXX include oam when we get the installation sorted out
DIRS = mosy getid getmany getnext getone getroute setany snmp in.snmpd snmpstat \
	trap hostmibd

all: 
	@for i in ${DIRS}; do \
		cd $$i; \
		$(MAKE) -f $$i.mk all ${MAKEARGS} ; \
		cd ..; \
	done

install: all
	@for i in ${DIRS}; do \
		cd $$i; \
		$(MAKE) -f $$i.mk $@ ${MAKEARGS} ; \
		cd ..; \
	done

clean clobber:
	@for i in ${DIRS}; do \
		cd $$i; \
		$(MAKE) -f $$i.mk $@ ${MAKEARGS} ; \
		cd ..; \
	done
