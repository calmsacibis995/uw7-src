#ident	"@(#)htable.mk	1.2"
#ident	"$Header$"

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#


# 
# 
# 		Copyright Notice 
# 
# Notice of copyright on this source code product does not indicate 
# publication.
# 
# 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
#

include $(CMDRULES)

LOCALDEF=	-DSYSV -DSTRNET -DBSD_COMP
INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

LDLIBS=		-lsocket -lnsl
YFLAGS=		-d

OBJS=		htable.o parse.o scan.o

all:		htable


install:	htable
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) htable

htable:		$(OBJS)
		$(CC) -o htable $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

y.tab.h:	parse.y
		$(YACC) $(YFLAGS) parse.y
		rm y.tab.c

clobber:	clean
		rm -f htable
clean:
		rm -f $(OBJS) y.tab.h parse.c scan.c

lintit:

FRC:

#
# Header dependencies
#

htable.o:	htable.c \
		$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/errno.h \
		$(INC)/netdb.h \
		htable.h \
		$(INC)/sys/types.h \
		$(INC)/netinet/in.h \
		$(INC)/sys/socket.h \
		$(INC)/arpa/inet.h \
		$(FRC)

parse.o:	parse.y \
		htable.h \
		$(INC)/sys/types.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(FRC)

scan.o:	scan.l \
		htable.h \
		$(INC)/sys/types.h \
		$(INC)/netinet/in.h \
		y.tab.h \
		$(FRC)

lex.yy.o:	lex.yy.c \
		y.tab.h \
		$(FRC)
