# @(#)db.mk	1.4

include $(LIBRULES)

HDIR=		.
DBHDRDIR=	../include
LOCALINC=	-I$(HDIR) -I$(DBHDRDIR)
LOCALDEF=	-D__DBINTERFACE_PRIVATE

OBJECTS=	db.o 

LIBRARY=	db.a 

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


#
# Header dependencies
#
db.o:		db.c \
		$(DBHDRDIR)/db.h 
