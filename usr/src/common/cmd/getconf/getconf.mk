
#ident	"@(#)getconf:getconf.mk	1.1"

#	Makefile for getconf 

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

MAKEFILE = getconf.mk


MAINS = getconf

OBJECTS =  getconf.o

SOURCES =  getconf.c

all:		$(MAINS)

getconf:		getconf.o 
	$(CC) -o $@  getconf.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)


getconf.o:	 $(INC)/sys/types.h \
	 $(INC)/stdio.h \
	 $(INC)/stdlib.h \
	 $(INC)/string.h \
	 $(INC)/unistd.h \
	 $(INC)/locale.h \
	 $(INC)/pfmt.h \
	 $(INC)/limits.h \
	 $(INC)/errno.h

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(MAINS)

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(INSDIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
	sed 's;^;$(INSDIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)
