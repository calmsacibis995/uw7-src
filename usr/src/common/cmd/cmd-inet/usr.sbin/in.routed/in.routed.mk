#ident	"@(#)in.routed.mk	1.3"
#ident	"$Header$"

include $(CMDRULES)

# dme - -W0,-2A1 gets alloca()
LOCALDEF=	 -DSYSV -DSTRNET -Dunixware \
		 -DSTATIC=static -W0,-2A1 -I.
INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

LDLIBS=		-lsocket -lnsl -lgen

OBJS=		if.o input.o main.o md5.o output.o parms.o radix.o \
		rdisc.o table.o trace.o

all:		in.routed rtquery

in.routed:	$(OBJS)
		$(CC) $(CFLAGS) -o in.routed $(OBJS) $(LDLIBS) $(SHLIBS)

rtquery:	rtquery.o md5.o
		$(CC) $(CFLAGS) -o rtquery rtquery.o md5.o $(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) in.routed
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) rtquery

clean:
		rm -f $(OBJS) rtquery.o

clobber:	clean
		rm -f in.routed rtquery

lintit:
		$(LINT) $(LINTFLAGS) *.c

