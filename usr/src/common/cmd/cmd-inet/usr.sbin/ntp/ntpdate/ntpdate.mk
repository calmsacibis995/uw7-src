#ident "@(#)ntpdate.mk	1.4"

include $(CMDRULES)

LOCALDEF=	-DSYS_UNIXWARE2 -DHAVE_TERMIOS \
		-DCONFIG_FILE=\"/etc/inet/ntp.conf\" -DDEBUG -DREFCLOCK
LOCALINC=	-I../include
LIB=		../lib/libntp.a
LDLIBS=		$(LIB) -lnsl -lsocket -lgen
PROGRAM=	ntpdate
INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

SRCS=	ntpdate.c version.c
OBJS = $(SRCS:.c=.o)

all:	$(PROGRAM)

install: all
	$(INS) -f $(INSDIR) -m 055 -u $(OWN) -g $(GRP) $(PROGRAM)

$(PROGRAM): $(OBJS) $(LIB)
	$(CC) -o $(PROGRAM) $(LDFLAGS) $(OBJS) $(LDLIBS)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROGRAM)

$(LIB):
	cd ../lib && $(MAKE) -f lib.mk

lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)
