#ident "@(#)authstuff.mk	1.5"

include $(CMDRULES)

LOCALDEF=	-DSYS_UNIXWARE2 -DHAVE_TERMIOS \
		-DCONFIG_FILE=\"/etc/inet/ntp.conf\" -DDEBUG -DREFCLOCK
LOCALINC=	-I../include
LIB=		../lib/libntp.a
LDLIBS=		$(LIB) -lnsl -lsocket -lgen

INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

all:	ntp_authspeed

ntp_authspeed: authspeed.o $(LIB)
	$(CC) -o $@ $(LDFLAGS) authspeed.o $(LDLIBS)

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) ntp_authspeed

clean:
	rm -f authspeed.o

clobber: clean
	rm -f ntp_authspeed

../lib/libntp.a:
	cd ../lib && $(MAKE) -f lib.mk
