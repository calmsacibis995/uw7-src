#ident  "@(#)nbtstat.mk	1.5"
#
# Copyrighted as an unpublished work.
# (c) Copyright 1996 SCO
# All rights reserved.
#
#      SCCS IDENTIFICATION

include $(CMDRULES)

PROGRAM=	nbtstat
LDLIBS=	-lnsl -lsocket -lgen $(LIB)
#LIBS= -lnsl -lsocket -ll -ly
#LOCALINC=-I$(ROOT)/$(MACH)/usr/include
INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin


.MUTEX: y.tab.h parser.c

SRCS=dnld.c lex.yy.c main.c nbstatus.c

OBJS=main.o lexor.o parser.o dnld.o nbstatus.o

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(PROGRAM)
	$(INS) -f $(ETC) -m 0444 -u root -g bin lmhosts.samp

all: nbtstat

nbtstat: $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o nbtstat $(OBJS) $(LDLIBS)

clean:
	-rm -f $(OBJS) y.tab.c y.tab.h lmhosts parser.c

clobber: clean
	rm -f nbtstat

y.tab.h parser.c : parser.y
	$(YACC) -d parser.y
	mv y.tab.c parser.c

lexor.o: y.tab.h

#unixnet.o nbstatus.o: nbxti.h

lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)
