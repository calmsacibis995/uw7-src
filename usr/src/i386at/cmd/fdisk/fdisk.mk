#ident	"@(#)fdisk:i386at/cmd/fdisk/fdisk.mk	1.15.1.1"

include	$(CMDRULES)

MAINS	= fdisk fdisk.boot
MSGS	= fdisk.str
OBJS	= fdisk.o bootcod.o
BOBJS	= $(OBJS) fstub.o

ALL:		$(MAINS)

fdisk:  $(OBJS)
	$(CC) $(LDFLAGS) -o fdisk $(OBJS) -lgen -ladm

fdisk.boot:  $(BOBJS)
	$(CC) $(LDFLAGS) -o fdisk.boot $(BOBJS) -lgen

# To make a new master bootstrap, "make -f fdisk.mk masterboot"

masterboot:
	$(HCC) -o bin2c bin2c.c $(LIBELF)
	$(AS) bootstrap.s
	echo 'text=V0x600 L446;' >tmp_mapfile
	$(LD) -o bootstrap.obj -M tmp_mapfile -dn bootstrap.o
	rm tmp_mapfile
	./bin2c bootstrap.obj bootcod.c

fdisk.c:            \
		 $(INC)/sys/types.h \
		 $(INC)/sys/vtoc.h \
		 $(INC)/sys/termios.h \
		 $(INC)/sys/fdisk.h \
		 $(INC)/curses.h \
		 $(INC)/term.h \
		 $(INC)/fcntl.h \
		 $(INC)/libgen.h

clean:
	rm -f $(BOBJS) bin2c bootstrap.o bootstrap.obj

clobber:        clean
	rm -f $(MAINS)

all : ALL

install: ALL $(MSGS)
	$(INS) -f $(USRSBIN) fdisk
	$(INS) -f $(USRSBIN) fdisk.boot
	-[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 fdisk.str
