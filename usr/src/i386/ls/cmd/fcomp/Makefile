#	copyright	"%c%"

#ident	"@(#)Makefile	1.2"

include $(CMDRULES)

all: fcomp

fcomp: fcomp.o
	$(CC) $(CFLAGS) -o $@ $@.o $(LDFLAGS) $(LDLIBS)

fcomp.o: fcomp.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h

clean:
	-rm -f *.o

clobber: clean
	-rm -f fcomp

install: all
	$(INS) -f $(ROOT)/$(MACH)/usr/lbin -m 0555 -u bin -g bin fcomp

strip:	all
	$(STRIP) fcomp
