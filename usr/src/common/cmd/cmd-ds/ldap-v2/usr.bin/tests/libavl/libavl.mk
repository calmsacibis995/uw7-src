# @(#)libavl.mk	1.4


LDAPTOP=	../../..

include $(CMDRULES)
include $(LDAPTOP)/local.defs

HDIR=		$(LDAPTOP)/include
LOCALINC=	-I$(HDIR) -I.

LDIR=		$(LDAPTOP)/usr.lib
LDLIBS=		-lavl 

TESTS=		libavl_test
SOURCES=	libavl_test.c


all:	$(TESTS)

libavl_test:	$$@.o
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
avl.o:	avl.c \
	$(LDAPTOP)/include/avl.h
