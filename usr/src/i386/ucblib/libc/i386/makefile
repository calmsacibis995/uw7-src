#ident	"@(#)ucb:i386/ucblib/libc/i386/makefile	1.3"
#ident	"$Header$"
#	copyright	"%c%"


#
#  makefile for ucblibc/i386
#
#

include $(LIBRULES)

MAC=
M4=m4 m4.def
PROF=
NONPROF=
SDEFLIST=
ARFLAGS = q

OBJECTS=

POBJECTS=

ALL:
	cd gen; $(MAKE) 
	cd sys; $(MAKE) 

all:	ALL

archive:
	#
	# Note that "archive" is invoked with libc/object as current directory.
	#
	$(LORDER) *.o|$(TSORT) >objlist
	$(AR) $(ARFLAGS) ../lib.libucb `cat objlist`
clean:
	-rm -f *.o

clobber: clean
	-rm -f */*.o
