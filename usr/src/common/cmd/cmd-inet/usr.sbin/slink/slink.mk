#ident	"@(#)slink.mk	1.2"
#ident  "$Header$"

include $(CMDRULES)

LOCALDEF=	-DSYSV -DSTRNET -DBSD_COMP -D_KMEMUSER -DSTATIC=
INSDIR=		$(USRSBIN)
OWN=		root
GRP=		bin

OBJS=		main.o parse.o exec.o builtin.o slink.o

all:		slink

install:	slink
		$(INS) -f $(INSDIR) -m 0500 -u $(OWN) -g $(GRP) slink

slink:		$(OBJS)
		$(CC) -o slink $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

clean:
		rm -f $(OBJS) lex.yy.c slink.c

clobber:	clean
		rm -f slink

lintit:
		$(LINT) $(LINTFLAGS) *.c
FRC:

#
# Header dependencies
#

builtin.o:	builtin.c \
		$(INC)/fcntl.h \
		$(INC)/sys/types.h \
		$(INC)/stropts.h \
		$(INC)/sys/stream.h \
		$(INC)/sys/socket.h \
		$(INC)/net/if.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/sockio.h \
		$(INC)/net/strioc.h \
		$(INC)/sys/dlpi.h \
		defs.h \
		$(FRC)

exec.o:		exec.c \
		$(INC)/varargs.h \
		$(INC)/stdio.h \
		defs.h \
		$(FRC)

main.o:		main.c \
		main.c \
		$(INC)/stdio.h \
		$(INC)/varargs.h \
		$(INC)/signal.h \
		defs.h \
		$(FRC)

parse.o:	parse.c \
		$(INC)/stdio.h \
		$(INC)/fcntl.h \
		$(INC)/varargs.h \
		$(INC)/sys/types.h \
		defs.h \
		$(FRC)

slink.o:	slink.l \
		defs.h \
		$(FRC)
