# @(#)hash.tests.mk	1.4

include $(CMDRULES)

DBTOP=		../..
HDIR=		.
DBHDRDIR=	$(DBTOP)/include
BTREEDIR=	$(DBTOP)/btree
LOCALINC=	-I$(HDIR) -I$(DBHDRDIR) -I$(BTREEDIR)

# The STATS line is to get hash and btree statistical use info.  This
# also forces ld to load the btree debug functions for use by gdb, which
# is useful.  The db library has to be compiled with -DSTATISTICS as well.
# The OORG line is for symbolic debug info in object code.

OORG=		-g
STATS=		-DSTATISTICS
LOCALDEF=	-D__DBINTERFACE_PRIVATE $(STATS) $(OORG)

OBJECTS=        dbtest.o strerror.o

LDIR=		$(DBTOP)
LDLIBS=         -ldb -lgen $(LDBMLIB)


TESTS=          driver2 tcreat3 tdel thash4 tread2 tseq tverify


all:    $(TESTS)

$(TESTS):	$$@.o
	$(CC) -o $@ $(LDFLAGS) $@.o -L$(LDIR) $(LDLIBS) $(SHLIBS)

install: all

clean:
	$(RM) -f *.o

clobber:        clean
	$(RM) -f $(TESTS)

lintit:
	$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) *.c
#
# Header dependencies
#

