# @(#)liblog.mk	1.2

LDAPTOP=	../..

include $(LIBRULES)
include $(LDAPTOP)/local.defs

HDIR=		$(LDAPTOP)/include
LOCALINC=	-I$(HDIR)

OBJECTS=   	ldaplog.o 
LIBRARY=	liblog.a


all:	$(LIBRARY)

$(LIBRARY):	$(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS); \
	$(RM) -f ../$@; \
	$(LN) -s liblog/$@ ../$@;

.c.o:
	$(CC) $(CFLAGS) -Kpic $(INCLIST) $(DEFLIST) -c $<


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

ldaplog.o:	ldaplog.c \
	$(LDAPTOP)/include/ldaplog.h
