# @(#)liblthread.mk	1.4


LDAPTOP=	../..

include $(LIBRULES)
include $(LDAPTOP)/local.defs

HDIR=		$(LDAPTOP)/include
LOCALINC=	-I$(HDIR) 


LIBRARY=	liblthread.a
OBJECTS=   	thread.o stack.o 

all:	$(LIBRARY)

$(LIBRARY):	$(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS); \
	$(RM) -f ../$@; \
	$(LN) -s liblthread/$@ ../$@;

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
thread.o:	thread.c \
		$(INC)/ldap.h \
		$(INC)/lber.h

stack.o:	stack.c \
		$(LDAPTOP)/include/lthread.h
