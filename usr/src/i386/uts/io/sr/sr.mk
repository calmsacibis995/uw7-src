#	Copyright (c) 1993 UNIVEL

#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)sr.mk	10.1"
#ident	"$Header$"

include $(UTSRULES)

LOCALDEF = -DDL_STRLOG 

KBASE =		../..
SR =		$(CONF)/pack.d/sr/Driver.o

SRFILES = \
	sr_str.o \
	sr_wait.o \
	sr_hash.o

CFILES =  \
	sr_str.c \
	sr_wait.c \
	sr_hash.c

all:	ID $(SR)

$(SR):	$(SRFILES)
	$(LD) -r -o $@ $(SRFILES)

#
# Configuration Section
#
ID:
	cd sr.cf; $(IDINSTALL) -R$(CONF) -M sr

#
# Header Install Section
#
headinstall: \
	$(KBASE)/io/sr/sr.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/sr/sr.h

clean:
	-rm -f *.o

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d sr

#
# Special Header dependencies
#

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

