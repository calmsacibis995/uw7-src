#ident  "hash.mk 1.0"

include $(LIBRULES)

HDIR=		.
DBHDRDIR=	../include
LOCALINC=	-I$(HDIR) -I$(DBHDRDIR)
LOCALDEF=	-D__DBINTERFACE_PRIVATE
OBJECTS=	hash.o hash_bigkey.o hash_buf.o hash_func.o hash_log2.o \
		hash_page.o hsearch.o ndbm.o

LIBRARY=	hash.a 

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
hash.o: 	hash.c \
		$(DBHDRDIR)/db.h \
		hash.h \
		page.h \
		extern.h

hash_bigkey.o:	hash_bigkey.c \
		$(DBHDRDIR)/db.h \
		hash.h \
		page.h \
		extern.h

hash_buf.o:	hash_buf.c \
		$(DBHDRDIR)/db.h \
		hash.h \
		page.h \
		extern.h

hash_func.o:	hash_func.c \
		$(DBHDRDIR)/db.h 

hash_log2.o:	hash_log2.c

hash_page.o:	hash_page.c \
		$(DBHDRDIR)/db.h \
		hash.h \
		page.h \
		extern.h

hsearch.o:	hsearch.c \
		$(DBHDRDIR)/db.h \
		search.h

ndbm.o:		ndbm.c \
		hash.h 
