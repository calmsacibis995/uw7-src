#ident "@(#)Makefile	1.3"
#ident "$Header$"

include $(CMDRULES)

CSRCS=	main.c getinfo.c debug.c send.c skip.c list.c subr.c
SRCS=	$(CSRCS) commands.l
OBJS=	main.o getinfo.o debug.o send.o skip.o list.o subr.o commands.o
RES		= ../../res/libresolv.a
LDLIBS		= $(RES) -ll -lsocket -lnsl
OWN=	bin
GRP=	bin

LOCALDEF =  	-DSVR4 -DUSE_POSIX
LOCALINC =	-I../../include

.MUTEX: lex.yy.c commands.c

all: nslookup

install: nslookup
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) nslookup
	$(INS) -f $(USRLIB) -m 0444 -u $(OWN) -g $(GRP) nslookup.help

nslookup: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDLIBS) $(PERFLIBS)

clean:
	rm -f $(OBJS) commands.c lex.yy.c lex.yy.o

clobber : clean
	rm -f nslookup

commands.c: commands.l
	$(LEX) -t $< > $@ || rm $@
