#ident	"@(#)lex:i386/makefile	1.9"

include $(CMDRULES)

LEXDIR=$(DFLTCLIB)/lex
INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

SOURCES=$(COMDIR)/main.c $(COMDIR)/sub1.c $(COMDIR)/sub2.c $(COMDIR)/header.c $(COMDIR)/parser.y
OBJECTS=main.o sub1.o sub2.o header.o y.tab.o
PRODUCTS=lex

all:	$(PRODUCTS)

lex:	$(OBJECTS)
	$(CC) $(OBJECTS) $(LINK_MODE) $(LDLIBS) $(LIBSGS) -ly -o lex 

main.o:	$(COMDIR)/main.c $(COMDIR)/ldefs.c $(COMDIR)/once.c
	$(CC) $(CFLAGS) -c -DCNAME=\"$(LEXDIR)/ncform\" \
	-DRATNAME=\"$(LEXDIR)/nrform\" -I$(INC) -I$(CPUINC) $(COMDIR)/main.c

sub1.o:	$(COMDIR)/sub1.c $(COMDIR)/ldefs.c
	$(CC) $(CFLAGS) -c -I$(INC) -I$(CPUINC) $(COMDIR)/sub1.c

sub2.o:	$(COMDIR)/sub2.c $(COMDIR)/ldefs.c
	$(CC) $(CFLAGS) -c -I$(INC) -I$(CPUINC) $(COMDIR)/sub2.c

header.o:	$(COMDIR)/header.c $(COMDIR)/ldefs.c
	$(CC) $(CFLAGS) -c -I$(INC) -I$(CPUINC) $(COMDIR)/header.c

y.tab.o:	$(COMDIR)/parser.y
		$(YACC) $(COMDIR)/parser.y
		$(CC) $(CFLAGS) -c -I$(INC) -I$(CPUINC) -I$(COMDIR) y.tab.c

install:	all
		cp lex lex.bak
		$(STRIP) lex
		/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)lex lex
		mv lex.bak lex

lintit:	$(SOURCES)	
	$(LINT) $(LINTFLAGS) -I$(INC) -I$(CPUINC) $(SOURCES)

clean:
		-rm -f $(OBJECTS)

clobber:	clean
		-rm -f y.tab.c
		-rm -f $(PRODUCTS)
