# @(#)test.mk	1.4

include $(CMDRULES)

HDIR=           .
DBHDRDIR=       ../include
LOCALINC=       -I$(HDIR) -I$(DBHDRDIR)

# The STATS line is to get hash and btree statistical use info.  This
# also forces ld to load the btree debug functions for use by gdb, which
# is useful.  The db library has to be compiled with -DSTATISTICS as well.
# The OORG line is for symbolic debug info in object code.

OORG=		-g
STATS=		-DSTATISTICS
LOCALDEF=       -D__DBINTERFACE_PRIVATE $(STATS) $(OORG)

OBJECTS=        dbtest.o strerror.o 




all:		dbtest

dbtest:		$(OBJECTS)
		$(CC) -o dbtest  $(LDFLAGS) $(OBJECTS) ../libdb.a \
			$(SHLIBS)

install: all

clean:		
		$(RM) -f $(OBJECTS)

clobber:	clean
		$(RM) -f dbtest

lintit:		
		$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) *.c

FRC:
