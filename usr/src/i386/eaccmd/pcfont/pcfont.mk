#	copyright	"%c%"

#ident	"@(#)pcfont.mk	1.3"

include $(CMDRULES)

OBJECTS = pcfont.o loadfont.o

all: pcfont pcfont.dy

pcfont: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(NOSHLIBS)

pcfont.dy: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) 

pcfont.o:	pcfont.c \
		$(INC)/sys/types.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/errno.h \
		$(INC)/sys/at_ansi.h \
		$(INC)/sys/kd.h \
		$(INC)/sys/uio.h \
		$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/string.h \
		$(INC)/stdlib.h \
		$(INC)/locale.h \
		$(INC)/unistd.h \
		$(INC)/fcntl.h \
		$(INC)/pfmt.h \
		pcfont.h

loadfont.o:	loadfont.c \
		$(INC)/sys/types.h \
		$(INC)/sys/at_ansi.h \
		$(INC)/sys/kd.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/ctype.h \
		$(INC)/string.h \
		$(INC)/stdlib.h \
		$(INC)/fcntl.h \
		pcfont.h

clean:
	-rm -f $(OBJECTS)

clobber: 	clean
	-rm -f pcfont pcfont.dy

install:	all
	$(INS) -f $(ROOT)/$(MACH)/sbin -m 555 -u bin -g bin pcfont
	$(INS) -f $(ROOT)/$(MACH)/usr/bin -m 555 -u bin -g bin pcfont.dy

strip:	all
	$(STRIP) pcfont
	$(STRIP) pcfont.dy
