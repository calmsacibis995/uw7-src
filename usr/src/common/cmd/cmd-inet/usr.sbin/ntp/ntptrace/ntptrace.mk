#ident "@(#)ntptrace.mk	1.4"

include $(CMDRULES)

PROGRAM=	ntptrace
LOCALDEF=	-DSYS_UNIXWARE2 -DHAVE_TERMIOS \
		-DCONFIG_FILE=\"/etc/inet/ntp.conf\" -DDEBUG -DREFCLOCK
LIB=		../lib/libntp.a
LDLIBS=		$(LIB) -lnsl -lsocket -lgen
LOCALINC=	-I../include
INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

SRCS=	ntptrace.c version.c
OBJS=	$(SRCS:.c=.o)

all:	$(PROGRAM)

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(PROGRAM)

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
