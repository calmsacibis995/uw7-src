#	copyright	"%c%"

#ident	"@(#)gettxt:gettxt.mk	1.4.6.1"

#	Makefile for gettxt

include $(CMDRULES)

OWN = bin
GRP = bin

LDLIBS=

MAKEFILE = gettxt.mk


MAINS = gettxt gettxt.dy

OBJECTS =  gettxt.o

SOURCES =  gettxt.c

all:		$(MAINS)

gettxt:		gettxt.o 
	$(CC) -o gettxt gettxt.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

gettxt.dy:	gettxt.o 
	$(CC) -o gettxt.dy gettxt.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)


gettxt.o:		 $(INC)/stdio.h \
			 $(INC)/locale.h \
			 $(INC)/pfmt.h \
			 $(INC)/string.h \
			 $(INC)/errno.h \
			 $(INC)/unistd.h \
			 $(INC)/stdlib.h

GLOBALINCS = $(INC)/stdio.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/errno.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

install: all
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) gettxt
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) gettxt.dy

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(DIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
	sed 's;^;$(DIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)
