# @(#)recno.mk	1.4

include $(LIBRULES)

HDIR=		.
DBHDRDIR=	../include
LOCALINC=	-I$(HDIR) -I$(DBHDRDIR)
LOCALDEF=	-D__DBINTERFACE_PRIVATE

OBJECTS=	rec_close.o rec_delete.o rec_get.o rec_open.o \
		rec_put.o rec_search.o rec_seq.o rec_utils.o

LIBRARY=	recno.a 

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
rec_close.o:	rec_close.c \
		$(DBHDRDIR)/db.h \
		recno.h \
		extern.h \
		../btree/btree.h \
		../btree/extern.h 

rec_delete.o:	rec_delete.c \
		$(DBHDRDIR)/db.h \
		recno.h \
		extern.h \
		../btree/btree.h \
		../btree/extern.h 

rec_get.o:	rec_get.c \
		$(DBHDRDIR)/db.h \
		recno.h \
		extern.h \
		../btree/btree.h \
		../btree/extern.h 

rec_open.o:	rec_open.c \
		$(DBHDRDIR)/db.h \
		recno.h \
		extern.h \
		../btree/btree.h \
		../btree/extern.h 

rec_put.o:	rec_put.c \
		$(DBHDRDIR)/db.h \
		recno.h \
		extern.h \
		../btree/btree.h \
		../btree/extern.h 

rec_search.o:	rec_search.c \
		$(DBHDRDIR)/db.h \
		recno.h \
		extern.h \
		../btree/btree.h \
		../btree/extern.h 

rec_seq.o:	rec_seq.c \
		$(DBHDRDIR)/db.h \
		recno.h \
		extern.h \
		../btree/btree.h \
		../btree/extern.h 

rec_utils.o:	rec_utils.c \
		$(DBHDRDIR)/db.h \
		recno.h \
		extern.h \
		../btree/btree.h \
		../btree/extern.h 
