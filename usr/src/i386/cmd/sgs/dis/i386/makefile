#ident	"@(#)dis:i386/makefile	1.7"
#
#	DIS MAKEFILE
#
include $(CMDRULES)

INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

HFILES= $(COMDIR)/structs.h $(COMDIR)/data32.h dis.h $(MACHINC)/paths.h $(MACHINC)/sgs.h $(COMINC)/libelf.h

SOURCES=$(COMDIR)/main.c $(COMDIR)/utls.c bits.c fcns.c tables.c \
	$(COMDIR)/extn.c $(COMDIR)/lists.c $(COMDIR)/debug.c

OBJECTS=extn.o main.o utls.o lists.o bits.o fcns.o tables.o debug.o 

PRODUCTS=dis

ARCH=AR32WR
ARFORMAT=PORTAR
I386=I386
DBO=RBO

DEFLIST=-D$(ARCH) -D$(I386) -D$(DBO) -D$(ARFORMAT) 

INCLIST=-I$(CPUINC) -I$(COMINC) -I. -I$(COMDIR) 

CC_CMD=$(CC) $(CFLAGS) -c $(DEFLIST) $(INCLIST)

all:	$(PRODUCTS)

$(PRODUCTS):	$(OBJECTS) $(LIBELF) $(LIBDWARF2)
	$(CC) -o dis $(OBJECTS) $(LINK_MODE) $(LIBELF) $(LIBDWARF2)

 
extn.o:		dis.h $(COMDIR)/structs.h $(COMDIR)/extn.c
	$(CC_CMD) $(COMDIR)/extn.c

main.o:		$(CPUINC)/paths.h $(CPUINC)/sgs.h \
		$(COMDIR)/structs.h $(COMDIR)/main.c
	$(CC_CMD) $(COMDIR)/main.c

utls.o:		dis.h $(CPUINC)/sgs.h $(COMDIR)/structs.h $(COMDIR)/utls.c
	$(CC_CMD) $(COMDIR)/utls.c

lists.o:	dis.h $(COMDIR)/structs.h $(CPUINC)/paths.h \
		$(CPUINC)/sgs.h $(COMDIR)/lists.c
	$(CC_CMD) $(COMDIR)/lists.c

debug.o:	dis.h $(COMDIR)/structs.h $(COMDIR)/data32.h $(CPUINC)/sgs.h \
		$(CPUINC)/sgs.h $(COMINC)/dwarf.h $(COMINC)/libdwarf2.h $(COMDIR)/debug.c
	$(CC_CMD) $(COMDIR)/debug.c

	
bits.o:		dis.h $(CPUINC)/sgs.h $(COMINC)/libelf.h bits.c
	$(CC_CMD) bits.c

fcns.o:		dis.h $(CPUINC)/sgs.h $(COMDIR)/structs.h fcns.c
	$(CC_CMD) fcns.c

tables.o:	dis.h tables.c
	$(CC_CMD) tables.c


install:	all
	cp dis dis.bak
	$(STRIP) dis
	sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)dis dis
	mv dis.bak dis


lintit:	$(SOURCES) $(HFILES)
	$(LINT) $(LINTFLAGS)  $(DEFLIST) $(INCLIST) $(SOURCES)

clean:
	rm -rf $(OBJECTS)

clobber: clean
	rm -f $(PRODUCTS)
