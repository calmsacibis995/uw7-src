#ident	"@(#)Makefile	11.1"

CFLAGS =

include		$(LIBRULES)

LDLIBS =	-L$(USRLIB)
SFLAGS =	-G -dy -h /usr/lib/libmail.so

LIB =		libmail.a
SLIB =		libmail.so

OBJS =		maillock.o

build:		$(LIB) $(SLIB)

$(LIB):		$(OBJS)
		$(AR) rv $@ $?

$(SLIB):	$(OBJS)
		$(CC) $(SFLAGS) -o $(SLIB) $(OBJS) $(LDLIBS)

install:
		$(INS) -f $(USRLIB) $(LIB)
		$(INS) -f $(USRLIB) $(SLIB)
		if [ ! -d $(USR)/include ] ; then mkdir -p $(USR)/include ; fi
		$(INS) -f $(USR)/include maillock.h

local-install:
		cp $(LIB) $(SLIB) /usr/lib
		cp maillock.h /usr/include

local-remove:
		rm -f /usr/lib/$(LIB) /usr/lib/$(SLIB)
		rm -f /usr/include/maillock.h

clean:
		rm -f $(OBJS)

clobber:	clean
		rm -f $(LIB) $(SLIB)
