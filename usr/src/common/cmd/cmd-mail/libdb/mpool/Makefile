#ident  "mpool.mk 1.0"

include $(LIBRULES)

HDIR=		.
DBHDRDIR=	../include
LOCALINC=	-I$(HDIR) -I$(DBHDRDIR)
LOCALDEF=	-D__DBINTERFACE_PRIVATE -DSTATISTICS

OBJECTS=	mpool.o 

LIBRARY=	mpool.a

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
mpool.o:	mpool.c \
		$(DBHDRDIR)/queue.h \
		$(DBHDRDIR)/db.h 
