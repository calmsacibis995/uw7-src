#ident	"@(#)nm:i386/makefile	1.6"

#
#
#	NM MAKEFILE - I386 VERSION
#
#
include $(CMDRULES)


INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

HFILES=$(CPUINC)/sgs.h $(CPUINC)/paths.h \
       $(COMINC)/libelf.h

SOURCES=$(COMDIR)/nm.c
OBJECTS=nm.o
PRODUCTS=nm

DEFLIST=-D$(ARFORMAT)
INCLIST=-I$(CPUINC) -I$(COMINC) 
CC_CMD=$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST)



all:	nm

nm: 	$(OBJECTS) $(LIBELF) $(LDLIBS)
	$(CC) $(OBJECTS) $(LIBELF) $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o nm

nm.o: 	$(HFILES) $(COMDIR)/nm.c
	$(CC_CMD) -c $(COMDIR)/nm.c 


install:	$(INSDIR)/$(SGS)nm 

$(INSDIR)/$(SGS)nm:	nm
	cp nm nm.bak
	$(STRIP) nm
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)nm nm
	mv nm.bak nm

lintit:	$(HFILES) $(SOURCES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $(SOURCES)

clean:
	-rm -f $(OBJECTS)

clobber: clean
	-rm -f $(PRODUCTS)

