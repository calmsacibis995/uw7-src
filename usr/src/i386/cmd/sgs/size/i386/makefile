#ident	"@(#)size:i386/makefile	1.7"
#	SIZE MAKEFILE
#
#
include $(CMDRULES)


INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

HFILES= $(COMINC)/filehdr.h $(COMINC)/ldfcn.h $(COMINC)/scnhdr.h \
	$(CPUINC)/sgs.h $(COMDIR)/process.h $(COMDIR)/defs.h machdep.h

SOURCES=$(COMDIR)/main.c $(COMDIR)/process.c $(COMDIR)/fcns.c

OBJECTS=main.o process.o fcns.o

PRODUCTS=size

DEFLIST=-D$(ARCH) -D$(ARFORMAT) $(FLEX) -DUNIX

INCLIST=-I. -I$(CPUINC) -I$(COMINC) -I$(COMDIR)

CC_CMD=$(CC) $(CFLAGS) -c $(DEFLIST) $(INCLIST)
#

all:	$(PRODUCTS)

$(PRODUCTS):	$(OBJECTS) $(LIBELF)
	$(CC) -o size $(OBJECTS) $(LIBELF) $(LDLIBS) $(LINK_MODE) $(LIBSGS)

lint:	$(OBJECTS) $(LIBELF)
	$(LINT) -o size $(OBJECTS) $(LIBELF) $(LDLIBS) 
#
main.o:	$(COMINC)/filehdr.h $(COMINC)/ldfcn.h $(CPUINC)/sgs.h \
	$(COMDIR)/defs.h machdep.h $(COMDIR)/main.c
	$(CC_CMD) $(COMDIR)/main.c
#
process.o:	$(COMINC)/filehdr.h $(COMINC)/scnhdr.h $(COMINC)/ldfcn.h \
		$(COMDIR)/process.h $(COMDIR)/defs.h $(COMDIR)/process.c \
		$(CPUINC)/sgs.h machdep.h
	$(CC_CMD) $(COMDIR)/process.c
#
fcns.o:	$(COMINC)/filehdr.h $(COMINC)/scnhdr.h $(COMINC)/ldfcn.h \
	$(CPUINC)/sgs.h $(COMDIR)/defs.h $(COMDIR)/fcns.c
	$(CC_CMD) $(COMDIR)/fcns.c
#
install: all
	cp size size.bak
	$(STRIP) size
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)size size
	mv size.bak size
#
lintit: $(SOURCES) $(HFILES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $(SOURCES)
#
clean:
	rm -f $(OBJECTS)
#
clobber: clean
	rm -f $(PRODUCTS)
