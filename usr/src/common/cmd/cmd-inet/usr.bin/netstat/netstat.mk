#ident	"@(#)netstat.mk	1.5"
#ident	"$Header$"

include $(CMDRULES)

# _KMEMUSER is required to ensure that some of the protocol stack internal
# structures used by netstat can be correctly declared (ie. atomic_int_t's).
LOCALDEF=	-D_KMEMUSER
INSDIR=		$(USRBIN)
OWN=		bin
GRP=		sys

LDLIBS=		-lsocket -lnsl $(LIBELF)
OBJS=		host.o if.o inet.o main.o mroute.o route.o unix.o

all:		netstat

netstat:	$(OBJS)
		$(CC) -o netstat  $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 02555 -u $(OWN) -g $(GRP) netstat

clean:
		rm -f $(OBJS) core a.out

clobber:	clean
		rm -f netstat

lintit:
		$(LINT) $(LINTFLAGS) *.c

FRC:
