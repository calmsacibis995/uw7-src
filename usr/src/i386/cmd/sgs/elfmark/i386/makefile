#ident	"@(#)elfmark:cmd/sgs/elfmark/i386/makefile	1.1"
#	elfmark MAKEFILE
#
#
include $(CMDRULES)


INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

HFILES= 

SOURCES=elfmark.c

OBJECTS=elfmark.o

PRODUCTS=elfmark

DEFLIST=

INCLIST=-I. -I$(CPUINC) -I$(COMINC) -I$(COMDIR)

CC_CMD=$(CC) $(CFLAGS) -c $(DEFLIST) $(INCLIST)
#

all:	$(PRODUCTS)

$(PRODUCTS):	$(OBJECTS) $(LIBELF)
	$(CC) -o elfmark $(OBJECTS) $(LDLIBS) $(LINK_MODE)

lint:	$(OBJECTS)
	$(LINT) -o elfmark $(OBJECTS) $(LDLIBS) 
#
elfmark.o:	elfmark.c
	$(CC_CMD) elfmark.c
#
install: all
	cp elfmark elfmark.bak
	$(STRIP) elfmark
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)elfmark elfmark
	mv elfmark.bak elfmark
#
lintit: $(SOURCES) $(HFILES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $(SOURCES)
#
clean:
	rm -f $(OBJECTS)
#
clobber: clean
	rm -f $(PRODUCTS)
