#		copyright	"%c%"

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved

#	This Module contains Proprietary Information of Microsoft
#	Corporation and should be treated as Confidential.

#ident	"@(#)cmd.mk	15.1"

include $(LIBRULES)

MAKEFILE =  libcmd.mk

LIBRARY  =  libcmd.a

OBJECTS	=  magic.o sum.o deflt.o getterm.o privname.o systbl.o sttyname.o \
	tp_ops.o

SOURCES	= $(OBJECTS:.o=.c)

all:	$(LIBRARY)


# local directory, nothing to copy

clean:
	-rm -f $(OBJECTS)

clobber: clean
	-rm -f $(LIBRARY)

FRC:

.PRECIOUS:	$(LIBRARY)

$(LIBRARY):	$(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)
