#	copyright	"%c%"

#ident	"@(#)eac:i386at/eaccmd/dosutil/dosslice/dosslice.mk	1.1.1.1"

include $(CMDRULES)

OWN = bin
GRP = bin

DOSOBJECTS = $(CFILES:.c=.o)
CMDS = dosslice 

.MUTEX: $(CMDS)


all: $(CMDS)

CFILES= dosslice.c

dosslice: dosslice.o
	$(CC) $(LDFLAGS) -o dosslice dosslice.o $(LDLIBS)

dosslice.o: dosslice.c \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/fdisk.h \
	$(INC)/fcntl.h \
	$(INC)/pwd.h

install: all
	 $(INS) -f $(USRBIN) -m 0711 -u $(OWN) -g $(GRP) dosslice
	
clean:
	rm -f *.o

clobber: clean
	rm -f $(CMDS)

lintit:
	$(LINT) $(LINTFLAGS) *.c
