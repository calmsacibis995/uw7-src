#ident	"@(#)mosy.mk	1.3"
#ident	"$Header$"
#
# Copyrighted as an unpublished work.
# (c) Copyright 1992 INTERACTIVE Systems Corporation
# All rights reserved.
#
#      @(#)mosy.mk	1.3

include $(CMDRULES)

LOCALDEF = -DSVR4
LOCALINC = -I$(INC)/netmgt
OBJS	=	asp.o mosy.o misc.o yacc.o
LDLIBS	=	-lsnmp -lm

# Post mosy utility stuff
POST_MOSY_OBJS = post_mosy.o
POST_MOSY_LIBS = -lsocket -lsmux -lsnmpio

all:		mosy post_mosy mibcomp

mosy:		asp.o mosy.o misc.o yacc.o
		${CC} -o mosy.dy ${LDFLAGS} ${OBJS} ${LDLIBS}
		${CC} -o mosy ${LDFLAGS} ${OBJS} ${LDLIBS} -dn

# Make a dynamic linked post_mosy.dy. Will be renamed to post_mosy when it is
# packaged.
post_mosy:      post_mosy.o
		$(CC) -o post_mosy.dy ${LOCALINC} ${LDFLAGS} ${POST_MOSY_OBJS} ${LDLIBS} ${POST_MOSY_LIBS}

mibcomp:	mibcomp.sh
		cp mibcomp.sh mibcomp

post_mosy.o:    post_mosy.c mosy.mk

asp.o:		asp.c mosy-defs.h
mosy.o:		mosy.c mosy-defs.h mosy.mk
misc.o:		misc.c mosy-defs.h
yacc.o:		yacc.c lex.include mosy-defs.h

yacc.c:		yacc.y
		-@echo "expect 23 shift/reduce and 11 reduce/reduce conflicts"
		yacc $(YACCFLAGS) yacc.y
		mv y.tab.c $@

lex.include:	lex.l
		$(LEX) $(LEXFLAGS) lex.l
		mv lex.yy.c $@

install:	all
		${INS} -f ${USRSBIN} -m 0555 -u ${OWN} -g ${GRP} mosy
		${INS} -f ${USRSBIN} -m 0555 -u ${OWN} -g ${GRP} mosy.dy
		${INS} -f ${USRSBIN} -m 0555 -u ${OWN} -g ${GRP} post_mosy.dy
		${INS} -f ${USRSBIN} -m 0555 -u ${OWN} -g ${GRP} mibcomp
	[ -d ${USRLIB}/locale/C/MSGFILES ] || \
		mkdir -p ${USRLIB}/locale/C/MSGFILES
	${INS} -f ${USRLIB}/locale/C/MSGFILES -m 0666 -u ${OWN} -g ${GRP} nmmosy.str
		
clean:
		rm -f $(OBJS) $(POST_MOSY_OBJS) mibcomp yacc.c lex.include *~

clobber:	clean
		rm -f mosy
