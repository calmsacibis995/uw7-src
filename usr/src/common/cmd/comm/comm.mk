#	copyright	"%c%"

#ident	"@(#)comm:comm.mk	1.3.6.3"
#	Makefile for comm

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

MAKEFILE = comm.mk

MAINS = comm

OBJECTS =  comm.o

SOURCES =  comm.c

all:		$(MAINS)

comm:	$(SOURCES)
	$(CC) $(CFLAGS) $(DEFLIST) -o comm comm.c $(LDFLAGS) $(LDLIBS) \
		$(PERFLIBS)

strip:
	$(STRIP) $(MAINS)
clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)


install: all
	$(INS) -f $(INSDIR)  -m 0555 -u $(OWN) -g $(GRP) $(MAINS)
