#ident	"@(#)inconfig.mk	1.3"
#ident	"$Header$"

include $(CMDRULES)

LOCALDEF=	-DSYSV -DSTRNET -DBSD_COMP -D_KMEMUSER
INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		sys

LDLIBS=		-lsocket -lnsl $(LIBELF)
OBJS=		inconfig.o

all:		inconfig

inconfig:	$(OBJS)
		$(CC) -o inconfig  $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 02555 -u $(OWN) -g $(GRP) inconfig
		$(INS) -f $(INSDIR) -m 02555 -u $(OWN) -g $(GRP) inconfig

clean:
		rm -f $(OBJS) core a.out

clobber:	clean
		rm -f inconfig

lintit:
		$(LINT) $(LINTFLAGS) *.c

FRC:
