#ident	"@(#)cr1.mk	1.2"
#ident  "$Header$"

include $(CMDRULES)

#
#  This is the makefile for the key management commands/routines.
#

OWN = root
GRP = sys

# Don't change the order of the following libraries! There is/was a
# duplicate symbol for _des_crypt() in libnsl which is incompatible
# with the one we want in libcrypt!
LDLIBS = -lcrypt -lnsl

ETCIAF = $(ETC)/iaf
ETCCR1 = $(ETCIAF)/cr1

LIBIAF = $(USRLIB)/iaf
LIBCR1 = $(LIBIAF)/cr1

VARIAF = $(VAR)/iaf
VARCR1 = $(VARIAF)/cr1

OAMBASE = $(USRSADM)/sysadm
OAMCR1 = $(OAMBASE)/add-ons/nsu/netservices/cr1

OAMPKG = $(VAR)/sadm/pkg
PKGSAV = $(OAMPKG)/nsu/save
PKGMI = $(PKGSAV)/intf_install

DIRS = $(ETCCR1) $(VARCR1) $(LIBCR1) $(OAMCR1) $(OAMPKG)/cr1 $(OAMPKG)/nsu \
	  $(PKGSAV) $(PKGMI)

all: cryptkey keymaster scheme

cryptkey: cryptkey.o failure.o send_msg.o xdr.o
	$(CC) -o cryptkey cryptkey.o failure.o send_msg.o xdr.o \
		$(LDFLAGS) $(LDLIBS) $(SHLIBS)

keymaster: keymaster.o failure.o send_msg.o xdr.o
	$(CC) -o keymaster keymaster.o failure.o send_msg.o xdr.o \
		$(LDFLAGS) $(LDLIBS) $(SHLIBS)

scheme: scheme.o avaid.o failure.o rw_msg.o xdr.o
	$(CC) -o scheme scheme.o avaid.o failure.o rw_msg.o xdr.o \
		-liaf -lcmd $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all $(DIRS)
	$(INS) -f $(USRSBIN) -m 0755 -u $(OWN) -g $(GRP) keymaster
	$(INS) -f $(USRBIN) -m 0755 -u $(OWN) -g $(GRP) cryptkey
	$(INS) -f $(LIBCR1) -m 0755 -u $(OWN) -g $(GRP) scheme
	$(INS) -f $(PKGMI) -m 0644 -u $(OWN) -g $(GRP) oam/cr1.mi
	$(INS) -f $(OAMCR1) -m 0644 -u $(OWN) -g $(GRP) oam/Help
	$(INS) -f $(OAMCR1) -m 0644 -u $(OWN) -g $(GRP) oam/Menu.cr1
	$(INS) -f $(OAMCR1) -m 0644 -u $(OWN) -g $(GRP) oam/Form.cryptkey
	$(INS) -f $(OAMCR1) -m 0644 -u $(OWN) -g $(GRP) oam/Form.start
	$(INS) -f $(OAMCR1) -m 0644 -u $(OWN) -g $(GRP) oam/Text.cryptkey
	$(INS) -f $(OAMCR1) -m 0644 -u $(OWN) -g $(GRP) oam/Text.setmkey
	$(INS) -f $(OAMCR1) -m 0644 -u $(OWN) -g $(GRP) oam/Text.start
	$(INS) -f $(OAMCR1) -m 0644 -u $(OWN) -g $(GRP) oam/Text.stop
	-rm -f $(ETCCR1).des
	-rm -f $(ETCCR1).enigma
	-rm -f $(VARCR1).des
	-rm -f $(VARCR1).enigma
	-rm -f cr1 $(LIBCR1).des
	-rm -f cr1 $(LIBCR1).enigma
	$(SYMLINK) /etc/iaf/cr1 $(ETCCR1).des
	$(SYMLINK) /etc/iaf/cr1 $(ETCCR1).enigma
	$(SYMLINK) /var/iaf/cr1 $(VARCR1).des
	$(SYMLINK) /var/iaf/cr1 $(VARCR1).enigma
	$(SYMLINK) /usr/lib/iaf/cr1 $(LIBCR1).des
	$(SYMLINK) /usr/lib/iaf/cr1 $(LIBCR1).enigma

$(DIRS): 
	- [ -d $@ ] || mkdir -p $@
	$(CH)chmod 755 $@
	$(CH)chgrp $(GRP) $@
	$(CH)chown $(OWN) $@

clean:
	-rm -f *.o

clobber: clean
	-rm -f cryptkey keymaster scheme

lintit: 
	$(LINT) $(LINTFLAGS) cryptkey.c failure.c send_msg.c xdr.c
	$(LINT) $(LINTFLAGS) keymaster.c failure.c send_msg.c xdr.c
	$(LINT) $(LINTFLAGS) scheme.c avaid.c failure.c rw_msg.c xdr.c

avaid.o: avaid.c \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/string.h \
	$(INC)/sys/stat.h \
	$(INC)/termio.h \
	$(INC)/sys/param.h \
	$(INC)/deflt.h \
	$(INC)/mac.h \
	$(INC)/ia.h \
	$(INC)/audit.h \
	$(INC)/errno.h \
	$(INC)/iaf.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/tp.h

crypt.o: crypt.c \
	$(INC)/crypt.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/rpc.h \
	cr1.h \
	$(INC)/crypt.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h

cryptkey.o: cryptkey.c \
	$(INC)/stdio.h \
	$(INC)/assert.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/stat.h \
	$(INC)/unistd.h \
	$(INC)/pfmt.h \
	$(INC)/locale.h \
	cr1.h \
	$(INC)/crypt.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	keymaster.h

failure.o: failure.c \
	cr1.h \
	$(INC)/crypt.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	keymaster.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/sys/stropts.h \
	$(INC)/unistd.h \
	$(INC)/ctype.h \
	$(INC)/fcntl.h \
	$(INC)/pwd.h \
	$(INC)/poll.h \
	$(INC)/sys/types.h \
	$(INC)/sys/times.h \
	$(INC)/pfmt.h

keymaster.o: keymaster.c \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h \
	$(INC)/string.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/utsname.h \
	$(INC)/crypt.h \
	cr1.h \
	$(INC)/crypt.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	keymaster.h \
	$(INC)/poll.h \
	$(INC)/pwd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/stat.h \
	$(INC)/wait.h \
	$(INC)/pfmt.h \
	$(INC)/locale.h

rw_msg.o: rw_msg.c \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/stropts.h \
	$(INC)/rpc/rpc.h \
	$(INC)/unistd.h \
	$(INC)/cr1.h \
	$(INC)/crypt.h \
	cr1.h \
	$(INC)/crypt.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	keymaster.h \
	scheme.h

scheme.o: scheme.c \
	$(INC)/pwd.h \
	$(INC)/stdio.h \
	$(INC)/assert.h \
	$(INC)/errno.h \
	$(INC)/string.h \
	$(INC)/rpc/rpc.h \
	$(INC)/sys/types.h \
	$(INC)/sys/utsname.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h \
	$(INC)/pfmt.h \
	$(INC)/locale.h \
	$(INC)/ia.h \
	$(INC)/mac.h \
	$(INC)/iaf.h \
	$(INC)/crypt.h \
	$(INC)/cr1.h \
	cr1.h \
	$(INC)/crypt.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	keymaster.h \
	scheme.h \
	$(INC)/sys/types.h \
	$(INC)/termio.h

send_msg.o: send_msg.c \
	cr1.h \
	$(INC)/crypt.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	keymaster.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/sys/stropts.h \
	$(INC)/unistd.h \
	$(INC)/ctype.h \
	$(INC)/fcntl.h \
	$(INC)/pwd.h \
	$(INC)/poll.h \
	$(INC)/sys/types.h \
	$(INC)/sys/times.h

xdr.o: xdr.c \
	$(INC)/stdio.h \
	cr1.h \
	$(INC)/crypt.h \
	$(INC)/sys/types.h \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	scheme.h
