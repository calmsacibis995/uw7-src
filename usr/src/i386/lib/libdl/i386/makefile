#ident	"@(#)libdl:i386/makefile	1.7"
#
# i386-specific makefile for libdl
#
#

include $(LIBRULES)

SGSBASE=../../../cmd/sgs
INS=$(SGSBASE)/sgs.install
INSDIR=
HFILES=
SOURCES= $(COMDIR)/libdl.c align.s
OBJECTS= align.o libdl.o
PRODUCTS=
DEFLIST=
COMDIR= ../common
LDFLAGS= -G -h /usr/lib/libdl.so.1

#
all: libdl.so

libdl.so:	$(OBJECTS)
	$(CC) -o libdl.so $(LDFLAGS) $(OBJECTS)

libdl.o:	$(COMDIR)/libdl.c
	$(CC) -c -KPIC $(CFLAGS) $(MODE) $(INCLIST) $(COMDIR)/libdl.c 

align.o:	align.s
	$(CC) -c align.s

install:	all
	if [ ! -d $(CCSLIB) ]; then mkdir -p $(CCSLIB); fi
	if [ ! -d $(USRLIB) ]; then mkdir -p $(USRLIB); fi
	/bin/sh $(INS) 644 $(OWN) $(GRP) $(CCSLIB)/libdl.so libdl.so
	/bin/sh $(INS) 644 $(OWN) $(GRP) $(USRLIB)/libdl.so.1 libdl.so

clean:
	rm -f *.o

clobber:	clean
	rm -f libdl.so
