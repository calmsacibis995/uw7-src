#	copyright	"%c%"

#ident	"@(#)ed:ed.mk	1.30.4.2"
#ident  "$Header$"

#	Makefile for ed

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

LDLIBS = -lcrypt -lgen -lw
LDLIBS1 = -Bstatic -lcrypt -Bdynamic -lgen -lw

#top#

MAKEFILE = ed.mk


MAINS = ed ed.dy

OBJECTS =  ed.o textmem.o

SOURCES =  ed.c textmem.c

all:		$(MAINS)

ed:		ed.o textmem.o
	$(CC) -o $@ ed.o textmem.o $(LDFLAGS) $(LDLIBS) 

ed.dy:		ed.o textmem.o
	$(CC) -o $@ ed.o textmem.o $(LDFLAGS) $(LDLIBS1) 

ed.o:		 $(INC)/limits.h $(INC)/ctype.h $(INC)/fcntl.h \
		 $(INC)/unistd.h $(INC)/regexpr.h $(INC)/setjmp.h \
		 $(INC)/signal.h $(INC)/stdio.h \
		 $(INC)/stdlib.h $(INC)/sys/fcntl.h	\
		 $(INC)/sys/signal.h $(INC)/sys/stat.h \
		 $(INC)/sys/termio.h $(INC)/sys/types.h \
		 $(INC)/termio.h $(INC)/ustat.h $(INC)/pfmt.h \
		 $(INC)/mac.h

textmem.o:	 $(INC)/stdlib.h $(INC)/sys/types.h $(INC)/sys/mman.h \
		 $(INC)/fcntl.h $(INC)/errno.h $(INC)/unistd.h

GLOBALINCS = $(INC)/limits.h $(INC)/ctype.h $(INC)/fcntl.h \
	$(INC)/regexpr.h $(INC)/setjmp.h $(INC)/signal.h \
	$(INC)/stdio.h $(INC)/stdlib.h $(INC)/sys/fcntl.h \
	$(INC)/sys/signal.h $(INC)/sys/stat.h \
	$(INC)/sys/termio.h $(INC)/sys/types.h \
	$(INC)/termio.h $(INC)/ustat.h $(INC)/pfmt.h $(INC)/mac.h \
	$(INC)/errno.h


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) ed
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) ed.dy
	$(CH)-rm -f $(INSDIR)/red
	$(CH)ln $(INSDIR)/ed $(INSDIR)/red

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
