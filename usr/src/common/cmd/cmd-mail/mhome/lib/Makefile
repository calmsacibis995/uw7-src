#ident	"@(#)Makefile	11.1"

CFLAGS =

LOCALINC =	-I../../libdb/include

include		$(LIBRULES)

LDLIBS =	-L$(USRLIB) ../../libdb/libdb.a
SFLAGS =	-G -dy -h /usr/lib/libmultihome.so

LIB =		libmultihome.a
SLIB =		libmultihome.so

OBJS =		multihome.o

build:		$(LIB) $(SLIB)

$(LIB):		$(OBJS)
		$(AR) rv $@ $?

$(SLIB):	$(OBJS)
		$(CC) $(SFLAGS) -o $(SLIB) $(OBJS) $(LDLIBS)

clean:
		rm -f $(OBJS)

clobber:	clean
		rm -f $(LIB) $(SLIB)
