#ident	"@(#)Makefile	11.3"

LOCALINC =	-I. -I../../libdb/include -DNEWDB -DNIS -D__svr4__ -DUNIXWARE2 \
		-DUnixWare7

include		$(CMDRULES)

LDLIBS =	-L../../libdb -L$(USRLIB) -ldb -lsocket -lresolv -lgen

OBJS =		alias.o arpadate.o clock.o collect.o conf.o convtime.o \
		daemon.o deliver.o domain.o envelope.o err.o headers.o \
		macro.o main.o map.o mci.o mime.o parseaddr.o queue.o \
		readcf.o recipient.o savemail.o srvrsmtp.o stab.o stats.o \
		sysexits.o trace.o udb.o usersmtp.o util.o version.o safefile.o

sendmail:	$(OBJS)
		$(CC) -o sendmail $(CFLAGS) $(DEFLIST) $(OBJS) $(LDFLAGS) $(LDLIBS)

build:		sendmail

clean:
		rm -f $(OBJS)

clobber:	clean
		rm -f sendmail
