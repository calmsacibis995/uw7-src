#ident	"@(#)Makefile	11.1"

SHELL	= /bin/sh

LOCALDEF =
LOCALINC =	-I../../sendmail/src -I../lib -I../../libdb/include

LIBS	=

include $(CMDRULES)

LDLIBS	=	-L$(USRLIB) ../../libdb/libdb.a ../lib/libmultihome.a

DESTDIR = 	$(ETC)/mail

TARGET =	multihome

# statement coverage instrumented version, user lprof
TARGETT =	multihomet

build:		$(TARGET)

SRC =		multihome.c

$(TARGET):	$(SRC) Makefile ../../libdb/libdb.a
		$(CC) $(LDFLAGS) -o $(TARGET) $(DEFLIST) $(SRC) $(LDLIBS) $(LIBS)

# must remove -s flag from LDFLAGS
$(TARGETT):	$(SRC) Makefile
		$(CC) -ql -o $(TARGETT) $(DEFLIST) $(SRC) $(LDLIBS) $(LIBS)

clean:

clobber: 	clean
		-rm -f $(TARGET) $(TARGETT)
