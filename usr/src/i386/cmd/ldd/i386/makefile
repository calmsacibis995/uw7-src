#ident	"@(#)ldd:i386/makefile	1.8"

# makefile for i386 version of ldd (List Dynamic Dependencies)

include $(CMDRULES)

CMDBASE=../..
SGSBASE=$(CMDBASE)/sgs

INS=$(CMDBASE)/install/install.sh
INSDIR=$(CCSBIN)

COMDIR=../common
CPUDIR=../$(CPU)
SOURCES= $(COMDIR)/ldd.c
HFILES= $(CPUDIR)/machdep.h
OBJECTS= ldd.o
COMINC=$(SGSBASE)/inc/common
INCLIST=-I. -I$(COMINC)
DEFLIST=
PRODUCTS=$(SGS)ldd
LIBELF=-lelf
LINK_MODE=

OBJECTS= ldd.o

all:	$(OBJECTS)
	$(CC) -o $(PRODUCTS) $(CFLAGS) $(LINK_MODE) $(OBJECTS) $(LIBELF)

ldd.o:	$(COMDIR)/ldd.c $(HFILES)
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMDIR)/ldd.c

install:	all
	cp $(PRODUCTS) $(PRODUCTS).bak
	$(STRIP) $(PRODUCTS)
	/bin/sh $(INS) -f $(INSDIR) $(PRODUCTS)
	/bin/sh $(INS) -f $(UW_CCSBIN) $(PRODUCTS)
	/bin/sh $(INS) -f $(OSR5_CCSBIN) $(PRODUCTS)
	mv $(PRODUCTS).bak $(PRODUCTS)

lintit:	$(SOURCES)
	$(LINT) $(LINTFLAGS) $(INCLIST) $(DEFLIST) $(SOURCES)

clean:
	-rm -f $(OBJECTS)

clobber:	clean
	-rm -f $(PRODUCTS)
