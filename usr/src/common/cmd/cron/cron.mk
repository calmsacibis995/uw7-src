#	copyright	"%c%"

# 	Portions Copyright(c) 1988, Sun Microsystems, Inc.
#	All Rights Reserved.

#ident	"@(#)cron:common/cmd/cron/cron.mk	1.51"

include $(CMDRULES)

INSDIR=$(USRBIN)
SPL=$(VAR)/spool
SPOOL=$(VAR)/spool/cron

CRONLIB=$(ETC)/cron.d
CRONSPOOL=$(SPOOL)/crontabs
ATSPOOL=$(SPOOL)/atjobs

XDIRS= $(ROOT) $(ETC) $(USR) $(INSDIR) $(LIB) $(SPL) $(SPOOL)\
      $(CRONLIB) $(CRONSPOOL) $(ATSPOOL)

DIRS= $(SPL) $(SPOOL) $(CRONLIB) $(CRONSPOOL) $(ATSPOOL)

CMDS= cron at atq atrm crontab batch

LDLIBS=-lcmd -liaf -lgen
ARFLAGS=cr
YFLAGS=-d

all:	att1.c
	$(MAKE) -f cron.mk $(MAKEARGS) $(CMDS)

install: all
	-rm -f $(ETC)/cron
	$(INS) -f $(USRSBIN) -m 2750 -u root -g sys cron
	-mkdir ./tmp
	-$(CP) cron.dfl ./tmp/cron
	$(INS) -f $(ETC)/default -m 444 -u root -g sys ./tmp/cron
	-rm -rf ./tmp
	$(SYMLINK) /usr/sbin/cron $(ETC)/cron
	$(INS) -f $(INSDIR) -m 2555 -u bin -g sys at
	$(INS) -f $(INSDIR) -m 2555 -u bin -g sys atrm
	$(INS) -f $(INSDIR) -m 2555 -u bin -g sys atq
	$(INS) -f $(INSDIR) -m 2555 -u bin -g sys crontab
	$(INS) -f $(INSDIR) -m 555 -u bin -g sys batch

libelm.a: elm.o
	$(AR) $(ARFLAGS) libelm.a elm.o

cron:	copylog.o cron.o cronfuncs.o libelm.a
	$(CC) copylog.o cron.o cronfuncs.o libelm.a -o $@ $(LDFLAGS) \
		$(LDLIBS) $(SHLIBS)

crontab:	crontab.o permit.o cronfuncs.o funcs.o
	$(CC) crontab.o permit.o cronfuncs.o funcs.o -o $@ $(LDFLAGS) $(LDLIBS) $(SHLIBS)

at:	at.o att1.o att2.o cronfuncs.o funcs.o permit.o
	$(CC) at.o att1.o att2.o cronfuncs.o funcs.o permit.o -o $@ $(LDFLAGS) \
		$(LDLIBS) $(SHLIBS)

atq:	atq.o  cronfuncs.o funcs.o permit.o
	$(CC) atq.o  cronfuncs.o funcs.o permit.o -o $@ $(LDFLAGS) $(LDLIBS) $(SHLIBS)

atrm:	atrm.o cronfuncs.o funcs.o permit.o
	$(CC) atrm.o cronfuncs.o funcs.o permit.o -o $@ $(LDFLAGS) $(LDLIBS) $(SHLIBS)

batch:	batch.sh
	cp batch.sh batch



att1.c att1.h:	att1.y
	$(YACC) $(YFLAGS) att1.y
	-mv y.tab.c att1.c
	mv y.tab.h att1.h

att2.c:	att2.l
	$(LEX) $(LFLAGS) att2.l
	ed - lex.yy.c < att2.ed
	mv lex.yy.c att2.c

att1.o:	att1.c att1.h

att2.o:	att2.c att1.h

copylog.o: copylog.c
cron.o:	cron.c cron.h
crontab.o:	crontab.c cron.h
at.o:	at.c cron.h
atrm.o:	atrm.c cron.h
atq.o:	atq.c cron.h

clean:
	rm -f *.o libelm.a att1.h att1.c att2.c

clobber:	clean
	rm -f $(CMDS)
