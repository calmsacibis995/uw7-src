# @(#)liblber.mk	1.4

LDAPTOP=	../../..

include $(CMDRULES)
include $(LDAPTOP)/local.defs

HDIR=		$(LDAPTOP)/include
LOCALINC=	-I$(HDIR) -I.

LDIR=		$(LDAPTOP)/usr.lib
LDLIBS=		-lavl -llber

TESTS=		liblber_dtest liblber_etest
SOURCES=	liblber_dtest.c liblber_etest.c


all:	$(TESTS)

$(TESTS):	$$@.o
	$(CC) -o $@ $(LDFLAGS) $@.o -L$(LDIR) $(LDLIBS) $(SHLIBS)

install: all

clean:
	$(RM) -f *.o  

clobber:	clean
	$(RM) -f $(TESTS)

lintit:
	$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) $(SOURCES)
#
# Header dependencies
#
decode.o:	decode.c \
		$(INC)/lber.h

encode.o:	encode.c \
		$(INC)/lber.h

io.o:		io.c \
		$(INC)/lber.h

bprint.o:	bprint.c \
		$(INC)/lber.h
