# @(#)libavl.mk	1.4

LDAPTOP=	../..

include $(LIBRULES)
include $(LDAPTOP)/local.defs

HDIR=		$(LDAPTOP)/include
LOCALINC=	-I$(HDIR)

OBJECTS=   	avl.o 
LIBRARY=	libavl.a


all:	$(LIBRARY)

$(LIBRARY):	$(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS); \
	$(RM) -f ../$@; \
	$(LN) -s libavl/$@ ../$@;

install: all

clean:
	$(RM) -f *.o 

clobber:	clean
	$(RM) -f $(LIBRARY) *.o ../$(LIBRARY)

lintit:
	$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) *.c

FRC:

#
# Header dependencies
#

avl.o:	avl.c \
	$(LDAPTOP)/include/avl.h
