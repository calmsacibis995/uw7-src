#ident	"@(#)m4:i386/makefile	1.12"

include $(CMDRULES)

INS= $(SGSBASE)/sgs.install
INSDIR= $(CCSBIN)

LOCALFLAG=-Kuchar

YACCLIB=-ly

HFILES=$(COMDIR)/m4.h
SOURCES=$(COMDIR)/m4.c $(COMDIR)/m4ext.c $(COMDIR)/m4macs.c $(COMDIR)/m4y.y
OBJECTS=m4.o m4ext.o m4macs.o m4y.o
PRODUCTS=m4

all:	
	if [ "$(NATIVE)" = "yes" ] ; \
	then \
		$(MAKE) LOCALFLAG="-Kuchar -DNATIVE" build ; \
	else \
		$(MAKE) build ; \
	fi

build:	$(PRODUCTS)

$(PRODUCTS):	$(OBJECTS)	
	$(CC) $(OBJECTS) $(YACCLIB) $(LDLIBS) $(LIBSGS) $(LINK_MODE) -o m4

m4.o:	$(COMDIR)/m4.c $(COMDIR)/m4.h $(CPUINC)/sgs.h
	$(CC) $(LOCALFLAG) -I$(CPUINC) $(CFLAGS) -c $(COMDIR)/m4.c

m4ext.o:	$(COMDIR)/m4ext.c $(COMDIR)/m4.h
	$(CC) $(LOCALFLAG) -I$(CPUINC) $(CFLAGS) -c $(COMDIR)/m4ext.c

m4macs.o:	$(COMDIR)/m4macs.c $(COMDIR)/m4.h
	$(CC) $(LOCALFLAG) -I$(CPUINC) $(CFLAGS) -c $(COMDIR)/m4macs.c

m4y.o:	$(COMDIR)/m4y.y
	$(YACC) $(COMDIR)/m4y.y
	$(CC) $(LOCALFLAG) $(CFLAGS) -I$(CPUINC) -c y.tab.c
	mv y.tab.o m4y.o

install:	all
	cp m4 m4.bak
	$(STRIP) m4
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)m4 m4
	mv m4.bak m4

lintit: $(SOURCES) $(HFILES)
	$(LINT) $(LINTFLAGS) $(SOURCES) > lint.out

clean:
	-rm -f $(OBJECTS)
	
clobber:	clean
	-rm -f y.tab.c
	-rm -f $(PRODUCTS)	
