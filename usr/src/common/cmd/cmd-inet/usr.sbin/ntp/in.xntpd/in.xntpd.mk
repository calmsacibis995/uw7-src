#ident "@(#)in.xntpd.mk	1.5"

include $(CMDRULES)

PROGRAM=	in.xntpd
LOCALDEF=	-DSYS_UNIXWARE2 -DHAVE_TERMIOS -DDEBUG \
		-DCONFIG_FILE=\"/etc/inet/ntp.conf\" -DREFCLOCK -DLOCAL_CLOCK
LOCALINC=	-I../include
LIB=		../lib/libntp.a
LDLIBS=		$(LIB) -lnsl -lsocket -lgen
INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

SRCS=	ntp_config.c ntp_control.c ntp_io.c ntp_leap.c \
	ntp_loopfilter.c ntp_monitor.c ntp_peer.c ntp_proto.c \
	ntp_refclock.c ntp_request.c ntp_restrict.c ntp_timer.c \
	ntp_unixclock.c ntp_util.c ntp_intres.c ntp_filegen.c ntpd.c \
	refclock_conf.c refclock_local.c version.c

OBJS=	$(SRCS:.c=.o)

all:	$(PROGRAM)

$(PROGRAM): $(OBJS) $(LIB)
	$(CC) -o $@ $(LDFLAGS) $(OBJS) $(LDLIBS)

install:	all
	$(INS) -f $(INSDIR) -m 555 -u $(OWN) -g $(GRP) $(PROGRAM)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROGRAM)

../lib/libntp.a:
	cd ../lib && $(MAKE) -f lib.mk

lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)
