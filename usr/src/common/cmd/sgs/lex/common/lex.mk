#ident	"@(#)lex:common/lex.mk	1.13"

ROOT=
CCSBIN=$(ROOT)/usr/ccs/bin
CCSLIB=$(ROOT)/usr/ccs/lib
SGS=
CC=cc
CFLAGS=-O
LIBELF=
LDLIBS=
LINK_MODE=
LINT=lint
LINTFLAGS=
INC=$(ROOT)/usr/include
INCSYS=$(ROOT)/usr/include

LEXDIR=$(CCSLIB)/lex
USRBIN=$(ROOT)/usr/bin
SGSBASE=../..
MACH=
INS=$(SGSBASE)/sgs.install
MACHINC=$(SGSBASE)/inc/$(MACH)
INSDIR=$(CCSBIN)

STRIP=strip
YACC=yacc

SOURCES=main.c sub1.c sub2.c header.c parser.y
OBJECTS=main.o sub1.o sub2.o header.o y.tab.o
PRODUCTS=lex

all:	$(PRODUCTS)

lex:	$(OBJECTS)
	$(CC) $(OBJECTS) $(LINK_MODE) $(LDLIBS) -ly -o lex 

main.o:	main.c ldefs.c once.c
	$(CC) $(CFLAGS) -c -DCNAME=\"$(LEXDIR)/ncform\" \
	-DRATNAME=\"$(LEXDIR)/nrform\" -I$(INC) -I$(MACHINC) main.c

sub1.o:	sub1.c ldefs.c
	$(CC) $(CFLAGS) -c -I$(INC) -I$(MACHINC) sub1.c

sub2.o:	sub2.c ldefs.c
	$(CC) $(CFLAGS) -c -I$(INC) -I$(MACHINC) sub2.c

header.o:	header.c ldefs.c
	$(CC) $(CFLAGS) -c -I$(INC) -I$(MACHINC) header.c

y.tab.o:	parser.y
		$(YACC) parser.y
		$(CC) $(CFLAGS) -c -I$(INC) -I$(MACHINC) y.tab.c

install:	all
		cp lex lex.bak
		$(STRIP) lex
		/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)lex lex
		mv lex.bak lex

lintit:	$(SOURCES)	
	$(LINT) $(LINTFLAGS) -I$(INC) -I$(MACHINC) $(SOURCES)

clean:
		-rm -f $(OBJECTS)

clobber:	clean
		-rm -f y.tab.c
		-rm -f $(PRODUCTS)
