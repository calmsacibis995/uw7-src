#	copyright	"%c%"

#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/dosformat.mk	1.5.3.1"
#ident  "$Header$"

include $(CMDRULES)


OWN = bin
GRP = dos

DIRS = $(USRBIN) $(ETC)/default

DOSOBJECTS = $(CFILES:.c=.o)
CMDS = dosformat 

.MUTEX: $(CMDS)


all: $(CMDS)

CFILES= bin2c.c 

dosformat: dosformat.o
	$(CC) -o dosformat dosformat.o  $(LDFLAGS) $(SHLIBS)

dosformat.o: dosformat.c \
	$(INC)/sys/types.h \
	$(INC)/unistd.h \
	MS-DOS.h \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/vtoc.h \
	$(INC)/fcntl.h \
	$(INC)/signal.h \
	MS-DOS_boot.h

MS-DOS_boot.h:	bootstrap.s bin2c
	rm -f bootstrap.obj bootstrap bootstrap.o
	as bootstrap.s
	echo	'text=V0x7c00;' > piftmp
	ld -o bootstrap.obj -Mpiftmp -dn bootstrap.o
	rm -f piftmp
	./bin2c bootstrap.obj MS-DOS_boot.h

bin2c:	bin2c.o
	cc -o bin2c bin2c.o -lelf



$(DOSOBJECTS): MS-DOS.h

install: $(DIRS) all
	 $(INS) -f $(USRBIN) -m 0711 -u $(OWN) -g $(GRP) dosformat
	
$(DIRS):
	[ -d $@ ] || mkdir $@ ;\
		$(CH)chmod 0755 $@ ;\
		$(CH)chown $(OWN) $@ ;\
		$(CH)chgrp bin $@

clean:
	rm -f *.o $(DOSOBJECTS)

clobber: clean
	rm -f $(CMDS)

lintit:
	$(LINT) $(LINTFLAGS) *.c
