#	copyright	"%c%"

#ident	"@(#)libcommon.mk	1.2"
#ident	"$Header$"
#
# Common makefile definitions and targets used by library makefiles
#


#####
#
# Each library makefile must define $(LIBNAME) before including
# this file, so that the object and lint library names can be
# defined. Also, the separate makefiles must define
#
#	SRCS	list of source files
#	OBJS	list of objects created from the source files
#	LINTS	list of other lint libraries needed to lint the library
#
# Also, each library makefile should either include the main common
# definition file (common.mk in the top level directory) or must take
# other steps to define common things like $(CFLAGS), $(LINT), etc.
#####

LIBRARY	=	lib$(LIBNAME).a
LINTLIB	=	llib-l$(LIBNAME).ln

all:		$(LIBRARY)

install:	all

clean::	
	$(RM) $(OBJS)

clobber:	clean
	$(RM) $(LIBRARY) $(LINTLIB)

strip:
	$(STRIP) $(LIBRARY)

library:	$(LIBRARY)

$(LIBRARY):	$(OBJS)
	$(AR) rv $(LIBRARY) $?

lint:
	$(LINT) $(CFLAGS) $(LINTFLAGS) $(LINTTXT)

lintlib:
	$(LINT) -abhmuvxy -o $(LIBNAME) $(CFLAGS) $(LINTFLAGS) $(LINTTXT)
