
#ident	"@(#)csplit:csplit.mk	1.4.3.2"
#ident	"$Header$"

#	Makefile for csplit

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

LDLIBS = -lgen

MAKEFILE = csplit.mk

MAINS = csplit

OBJECTS =  csplit.o

SOURCES =  csplit.c

all:		$(MAINS)

csplit:	$(SOURCES)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $(SOURCES) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

install: all
	$(INS) -f $(INSDIR)  -m 0555 -u $(OWN) -g $(GRP) $(MAINS)
