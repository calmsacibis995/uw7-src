# @(#)libldif.mk	1.4

LDAPTOP=	../..

include $(LIBRULES)
include $(LDAPTOP)/local.defs

HDIR=		$(LDAPTOP)/include
LOCALINC=	-I$(HDIR) 

LDIR=		$(LDAPTOP)/usr.lib


OBJECTS=   	line64.o

LIBRARY=libldif.a

all:	$(LIBRARY)

$(LIBRARY):	$(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS); \
	$(RM) -f ../$@; \
	$(LN) -s libldif/$@ ../$@;


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

line64.o:	line64.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		$(LDAPTOP)/include/ldif.h

