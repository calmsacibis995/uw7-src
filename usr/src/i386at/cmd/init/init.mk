#	copyright	"%c%"

#ident	"@(#)init:i386at/cmd/init/init.mk	1.8.4.20"

include $(CMDRULES)

OWN = root
GRP = sys

LOCALDEF = -DNO_TPCONS	# disable check for trusted path 
LDLIBS = -lcmd -lgen

MSGS = init.str

all: init

init: init.o 
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

init.o: init.c \
	$(INC)/sys/types.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/priv.h \
	$(INC)/mac.h \
	$(INC)/sys/uadmin.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h \
	$(INC)/string.h \
	$(INC)/utmpx.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/sys/tty.h \
	$(INC)/ctype.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h \
	$(INC)/time.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/param.h \
	$(INC)/audit.h \
	$(INC)/sys/systeminfo.h \
	$(INC)/limits.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/sad.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/secsys.h \
	$(INC)/sys/tp.h \
	$(INC)/deflt.h \
	$(INC)/sys/uadmin.h \
	$(INC)/sys/sysi86.h

install: all $(MSGS)
	-rm -f $(ETC)/init
	-rm -f $(ETC)/telinit
	-rm -f $(USRSBIN)/init
	$(INS) -f $(SBIN)    -o -m 0555 -u $(OWN) -g $(GRP) init
	$(INS) -f $(USRSBIN) -o -m 0555 -u $(OWN) -g $(GRP) init
	-cp init.dfl $(ETC)/default/init
	-$(SYMLINK) /sbin/init $(ETC)/init
	-$(SYMLINK) /sbin/init $(ETC)/telinit
	-[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 init.str

clean:
	rm -f init.o

clobber: clean
	rm -f init

lintit:
	$(LINT) $(LINTFLAGS) init.c
