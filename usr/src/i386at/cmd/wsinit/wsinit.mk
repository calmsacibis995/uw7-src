#	copyright	"%c%"

#ident	"@(#)wsinit.mk	1.8"

#
# 	wsinit.mk:
# 	makefile for the wsinit command
#

include	$(CMDRULES)

LOCALDEF = -D_LTYPES

MSGS	= wsinit.str

all:	wsinit wsinit.dy

install:	all $(MSGS)
		cp ./wstations.sh workstations
		$(INS) -f $(SBIN) -m 0554 -u 0 -g 3 ./wsinit
		$(INS) -f $(SBIN) -m 0554 -u 0 -g 3 ./wsinit.dy
		$(INS) -f $(ETC)/default -m 0644 -u 0 -g 3 ./workstations
		-rm -rf workstations
		-[ -d $(USRLIB)/locale/C/MSGFILES ] || \
			mkdir -p $(USRLIB)/locale/C/MSGFILES
		$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 wsinit.str

clean:
	rm -f *.o

clobber: clean
	rm -f workstations
	rm -f wsinit

wsinit.o:	wsinit.c \
	$(INC)/sys/genvid.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/stat.h

wsinit:	wsinit.o
	$(CC) -o wsinit wsinit.o $(LDFLAGS) $(ROOTLIBS)

wsinit.dy:	wsinit.o
	$(CC) -o wsinit.dy wsinit.o $(LDFLAGS)
