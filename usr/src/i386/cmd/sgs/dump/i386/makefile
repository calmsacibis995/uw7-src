#ident	"@(#)dump:i386/makefile	1.10"

#
#
#	DUMP MAKEFILE
#
#
include $(CMDRULES)

LINTFLAGS=
LINK_MODE=

LDLIBS=

INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

HFILES=$(COMDIR)/dumpmap.h $(COMDIR)/dump.h $(COMDIR)/data32.h \
       $(CPUINC)/sgs.h $(CPUINC)/paths.h \
       $(COMINC)/libelf.h $(COMINC)/dwarf.h \
	$(COMINC)/libdwarf2.h

SOURCES=$(COMDIR)/dump.c $(COMDIR)/fcns.c $(COMDIR)/debug.c \
	$(COMDIR)/dwarf2.c
OBJECTS=dump.o fcns.o debug.o dwarf2.o
PRODUCTS=dump

DEFLIST=-D$(ARFORMAT) -DELF_TARGET_ALL -DLONG_LONG
INCLIST=-I$(CPUINC) -I$(COMINC)
CC_CMD=$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST)



all:	dump

dump: 	$(OBJECTS) $(LDLIBS)
	$(CC) $(OBJECTS) $(LIBELF) $(LIBDWARF2) $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o dump

dump.o: 	$(HFILES) $(COMDIR)/dump.c
	$(CC_CMD) -c $(COMDIR)/dump.c 


fcns.o: $(HFILES) $(COMDIR)/fcns.c
	$(CC_CMD) -c $(COMDIR)/fcns.c

debug.o: $(HFILES) $(COMDIR)/debug.c
	$(CC_CMD) -c $(COMDIR)/debug.c

dwarf2.o:	$(HFILES) $(COMDIR)/dwarf2.c
	$(CC_CMD) -c $(COMDIR)/dwarf2.c

install:	$(INSDIR)/$(SGS)dump 

$(INSDIR)/$(SGS)dump:	dump
	cp dump dump.bak
	$(STRIP) dump
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)dump dump
	mv dump.bak dump

lintit:	$(HFILES) $(SOURCES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $(SOURCES)

clean:
	-rm -f $(OBJECTS)

shrink clobber: clean
	-rm -f $(PRODUCTS)

