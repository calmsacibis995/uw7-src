#ident "@(#)ntpq.mk	1.4"

include $(CMDRULES)

PROGRAM=	ntpq
LOCALDEF=	-DSYS_UNIXWARE2 -DHAVE_TERMIOS \
		-DCONFIG_FILE=\"/etc/inet/ntp.conf\" -DDEBUG -DREFCLOCK
LOCALINC=	-I../include
LIB=		../lib/libntp.a
LDLIBS=		$(LIB) -lnsl -lsocket -lgen

INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

SRCS=	ntpq.c ntpq_ops.c version.c
OBJS=	$(SRCS:.c=.o)


all:	$(PROGRAM)

install : all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(PROGRAM)

$(PROGRAM): $(OBJS) $(LIB)
	$(CC) -o $(PROGRAM) $(LDFLAGS) $(OBJS) $(LDLIBS)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROGRAM)

../lib/libntp.a:
	cd ../lib && $(MAKE) -f lib.mk

lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)
