# @(#)back-passwd.mk	1.4

LDAPTOP=	../../..

include $(LIBRULES)
include $(LDAPTOP)/local.defs

HDIR=		$(LDAPTOP)/include
SLAPDIR=	..
LOCALINC=	-I$(HDIR) -I$(SLAPDIR)

OBJECTS	= search.o config.o
LIBRARY	= libback-passwd.a

all:	$(LIBRARY)

install: all

$(LIBRARY):	$(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

clean:
	$(RM) -f *.o 

clobber:	clean
	$(RM) -f $(LIBRARY)

lintit:
	$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) *.c
FRC:

#
# Header dependencies
#

search.o: 	search.c \
		../slap.h \
	 	$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/portable.h 

config.o: 	config.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/portable.h 
