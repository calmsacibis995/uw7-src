#ident	"@(#)demangler:i386/makefile	1.6"

#
#	C++ demangler makefile
#
include $(CMDRULES)

LINTFLAGS=
LINK_MODE=

LDLIBS=

INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

HFILES=

SOURCES=$(COMDIR)/dem_main.c
OBJECTS=dem_main.o
PRODUCTS=c++filt

DEFLIST=-DDEM_MAIN
INCLIST=-I$(CPUINC) -I$(COMINC)
CC_CMD=$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST)

all:	$(PRODUCTS)

$(PRODUCTS): 	$(OBJECTS) $(LIBELF) $(LDLIBS)
	$(CC) $(OBJECTS) $(LIBELF) $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o $(PRODUCTS)

dem_main.o:	$(HFILES) $(COMDIR)/dem_main.c
	$(CC_CMD) -c $(?)

install:	$(INSDIR)/$(SGS)$(PRODUCTS)

$(INSDIR)/$(SGS)$(PRODUCTS): $(PRODUCTS)
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

