#ident  "btree.mk 1.0"

include $(LIBRULES)

HDIR=		.
DBHDRDIR=	../include
LOCALINC=	-I$(HDIR) -I$(DBHDRDIR)
LOCALDEF=	-D__DBINTERFACE_PRIVATE -DSTATISTICS -DDEBUG

OBJECTS=	bt_close.o bt_conv.o bt_debug.o bt_delete.o bt_get.o \
		bt_open.o bt_overflow.o bt_page.o bt_put.o bt_search.o \
		bt_seq.o bt_split.o  bt_utils.o

LIBRARY=	btree.a 

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
bt_close.o:	bt_close.c \
		$(DBHDRDIR)/db.h \
		btree.h

bt_conv.o:	bt_conv.c \
		$(DBHDRDIR)/db.h \
		btree.h

bt_debug.o:	bt_debug.c \
		$(DBHDRDIR)/db.h \
		btree.h

bt_delete.o:	bt_delete.c \
		$(DBHDRDIR)/db.h \
		btree.h

bt_get.o:	bt_get.c \
		$(DBHDRDIR)/db.h \
		btree.h

bt_open.o:	bt_open.c \
		$(DBHDRDIR)/db.h \
		btree.h

bt_overflow.o:	bt_overflow.c \
		$(DBHDRDIR)/db.h \
		btree.h

bt_page.o:	bt_page.c \
		$(DBHDRDIR)/db.h \
		btree.h

bt_put.o:	bt_put.c \
		$(DBHDRDIR)/db.h \
		btree.h

bt_search.o:	bt_search.c \
		$(DBHDRDIR)/db.h \
		btree.h

bt_seq.o:	bt_seq.c \
		$(DBHDRDIR)/db.h \
		btree.h

bt_split.o:	bt_split.c \
		$(DBHDRDIR)/db.h \
		btree.h

bt_utils.o:	bt_utils.c \
		$(DBHDRDIR)/db.h \
		btree.h

