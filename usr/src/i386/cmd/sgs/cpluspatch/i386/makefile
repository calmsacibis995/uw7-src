#ident	"@(#)cpluspatch:i386/makefile	1.10"

#
#	C++ patch makefile
#
include $(CMDRULES)

LINTFLAGS=
LINK_MODE=

LDLIBS=$(LIBELF)

INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSLIB)

HFILES=

SOURCES=$(COMDIR)/patch.c
OBJECTS=patch.o
PRODUCTS=patch

DEFLIST=-DSHOBJ_SUPPORT
INCLIST=-I. -I$(COMDIR) -I$(CPUINC)
CC_CMD=$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST)

all: $(PRODUCTS)

$(PRODUCTS): 	$(OBJECTS) $(LDLIBS)
	$(CC) $(OBJECTS) $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o $(PRODUCTS)

patch.o:	$(HFILES) $(COMDIR)/patch.c
	$(CC_CMD) -c $(COMDIR)/patch.c 

install : all
	$(MV) $(PRODUCTS) $(PRODUCTS).bak
	$(CP) $(PRODUCTS).bak $(PRODUCTS)
	$(STRIP) $(PRODUCTS)
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)$(PRODUCTS) $(PRODUCTS)
	$(MV) $(PRODUCTS).bak $(PRODUCTS)

lintit:	$(HFILES) $(SOURCES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $(SOURCES)

clean:
	-rm -f $(OBJECTS)

shrink clobber: clean
	-rm -f $(PRODUCTS)
