#ident	"@(#)tsort:i386/makefile	1.5"

include $(CMDRULES)


INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

SOURCES=$(COMDIR)/tsort.c

OBJECTS=tsort.o

PRODUCTS=tsort

all:	$(PRODUCTS)

$(PRODUCTS):	$(OBJECTS)
	$(CC) $(OBJECTS) $(LDLIBS) $(LINK_MODE) -o tsort

tsort.o:	$(COMDIR)/tsort.c
	$(CC) $(CFLAGS) -c $(COMDIR)/tsort.c

install:	all
	cp tsort tsort.bak
	$(STRIP) tsort
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)tsort tsort
	mv tsort.bak tsort

lintit:	$(SOURCES)
	$(LINT) $(LINTFLAGS) $(SOURCES)

clean:	
	-rm -f $(OBJECTS)

clobber:	clean
	-rm -f $(PRODUCTS)
