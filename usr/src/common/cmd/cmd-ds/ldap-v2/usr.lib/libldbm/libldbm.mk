#ident	"@(#)libldbm.mk	1.4"
#ident	"$Header$"

LDAPTOP=	../..

include $(LIBRULES)
include $(LDAPTOP)/local.defs

HDIR=		$(LDAPTOP)/include
DBHDIR=		../libdb/include
LOCALINC=	-I$(HDIR) -I$(DBHDIR)

OBJECTS=   	ldbm.o 
LIBRARY=	libldbm.a

all:	$(LIBRARY)

$(LIBRARY):	$(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS); \
	$(RM) -f ../$@; \
	$(LN) -s libldbm/$@ ../$@;

install: all

clean:	
	$(RM) -f *.o 

clobber:	clean
	$(RM) -f ../$(LIBRARY) $(LIBRARY)

lintit:
	$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) *.c

FRC:

#
# Header dependencies
#
ldbm.o:		ldbm.c \
		$(LDAPTOP)/include/ldbm.h \
		$(DBHDIR)/db.h
