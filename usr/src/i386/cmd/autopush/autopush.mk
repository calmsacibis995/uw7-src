#	copyright	"%c%"

#ident	"@(#)autopush.mk	1.2"
#
# autopush.mk:
# makefile for autopush(1M) command
#

include $(CMDRULES)
OWN=root
GRP=sys
INCSYS=$(INC)
MSGS = autopush.str

all:	autopush

install:	all $(MSGS)
		$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) autopush
		$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) autopush
		$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) autopush.dy
		-$(SYMLINK) /sbin/autopush $(ETC)/autopush
		[ -d $(ETC)/ap ] || mkdir $(ETC)/ap
		$(CH)chmod 755 $(ETC)/ap
		$(CH)chgrp $(GRP) $(ETC)/ap
		$(CH)chown $(OWN) $(ETC)/ap
		$(INS) -f $(ETC)/ap -m 444 -u $(OWN) -g $(GRP) chan.ap
		$(INS) -f $(ETC)/ap -m 444 -u $(OWN) -g $(GRP) chan.ap.flop
		[ -d $(USRLIB)/locale/C/MSGFILES ] || \
			mkdir -p $(USRLIB)/locale/C/MSGFILES
		$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 autopush.str

autopush:	autopush.o
		$(CC) -o autopush autopush.o $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)
		$(CC) -o autopush.dy autopush.o $(LDFLAGS) $(LDLIBS)

clean:
	-rm -f *.o

clobber: clean
	-rm -f autopush


autopush.o:	autopush.c \
		$(INCSYS)/sys/types.h \
		$(INCSYS)/sys/sad.h \
		$(INCSYS)/sys/conf.h \
		$(INC)/stdio.h \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/ctype.h \
		$(INC)/memory.h \
		$(INC)/priv.h
		$(CC) $(CFLAGS) $(DEFLIST) -c autopush.c 
