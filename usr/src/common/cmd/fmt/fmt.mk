#	copyright	"%c%"

#ident	"@(#)fmt:fmt.mk	1.3.5.4"
#     Makefile for fmt 

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

LDFLAGS = -s $(SHLIBS) 

CFLAGS = -O 

STRIP = strip

SIZE = size

#top#

MAKEFILE = fmt.mk

MAINS = fmt 

OBJECTS =  main.o fmtfile.o misc.o 

SOURCES =  main.c fmtfile.c misc.c 

LOCALINCS = misc.h _locale.h

all:          $(MAINS)

$(MAINS):	$(OBJECTS)
		$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)
	
fmtfile.o:      misc.h

misc.o:		misc.h

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

install:	all
	$(INS) -f $(INSDIR) -m 00555 -u $(OWN) -g $(GRP) fmt

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

#     These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(INSDIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | sed 's;^;$(INSDIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)

