#ident	"@(#)devio:devio/devio.mk	1.2"
#copyright	"%c%"
#ident  "$Header$"

# Makefile for devio

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN=bin
GRP=bin

LDLIBS = 

MAKEFILE = Makefile

MAINS = devio

OBJECTS =  devio.o

SOURCES =  devio.c

all:		$(MAINS)

devio:		$(OBJECTS)	
	$(CC) -o devio $(OBJECTS) $(LDFLAGS) $(NOSHLIBS) $(LDLIBS)
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 uxdevio.str

devio.o:	devio.c \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/locale.h \
		$(INC)/pfmt.h \
		$(INC)/stdio.h \
		$(INC)/stdlib.h \
		$(INC)/string.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/types.h \
		$(INC)/time.h \
		$(INC)/unistd.h

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) devio

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)
