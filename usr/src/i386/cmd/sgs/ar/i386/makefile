#ident	"@(#)ar:i386/makefile	1.7"
#***************************************************************************
#                                                                          *
#			UNIX Archive (ar) makefile			   *
#									   *
#***************************************************************************

include $(CMDRULES)

INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

ARFORMAT = PORTAR

HFILES = $(CPUINC)/sgs.h $(COMINC)/libelf.h

SOURCES=$(COMDIR)/ar.c

OBJECTS=ar.o

PRODUCTS=ar

DEFLIST	= -D$(ARFORMAT)
INCLIST	= -I$(COMINC) -I$(CPUINC)
CC_CMD=$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST)

all:	$(PRODUCTS)


$(PRODUCTS):	$(OBJECTS) $(LIBELF)
	$(CC) $(OBJECTS) $(LINK_MODE) $(LIBELF) $(LIBSGS) -o ar

ar.o:	$(HFILES) $(COMDIR)/ar.c
	$(CC_CMD) -c $(COMDIR)/ar.c

install: all	
	cp ar ar.bak
	$(STRIP) ar
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)ar ar
	mv ar.bak ar

lintit: $(SOURCES) $(HFILES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $(SOURCES)

clean:
	-rm -f $(OBJECTS)

clobber: clean
	-rm -f $(PRODUCTS)
