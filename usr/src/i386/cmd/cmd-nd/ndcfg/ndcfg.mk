#ident "@(#)ndcfg.mk	23.1"
#ident "$Header$"

#
#	Copyright (C) The Santa Cruz Operation, 1997
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(CMDRULES)

# set NCFGDEBUG to 0 (no debugging) or 1 (has debugging).  This affects:
# - ndcfg -D argument
# - #$debug token in bcfg file
# - debug command
NETISL=0
LOCALCFLAGS=-DNCFGDEBUG=1 -DNETISL=$(NETISL)

# if using debugging malloc "dmalloc" package uncomment next line,
# use -ldmalloc in link line below, and put dmalloc.h in this directory.
# note if you add -DDMALLOC_FUNC_CHECK you will need to change the order
# of the #include files in the resulting tmplex and tmpyacc files
# adding -DDMALLOC_STRDUP for our own strdup in cmdops.c is helpful too.
# CFLAGS=-g -Xc -D_POSIX_SOURCE -DDMALLOC

TARGET=ndcfg
# TARGET=ndcfg ndisl
OFILES=tmpbcfgyacc.o tmpbcfglex.o tmpcmdyacc.o tmpcmdlex.o bcfgops.o main.o resops.o cmdops.o ca.o elfops.o

all: $(TARGET)
#	$(MAKE) -f ndcfg.mk ndcfg NETISL=0 $(MAKEARGS)
#	$(MAKE) -f ndcfg.mk ndisl NETISL=1 $(MAKEARGS)

# ndcfg: clean $(OFILES)
ndcfg: $(OFILES)
# standard link line
	$(CC) -o ndcfg $(OFILES) -lx -lhpsl -lresmgr -lcurses -lelf -lcmd -lc
# Electric Fence debugging malloc package(no need to modify CFLAGS above):
#	$(CC) -g -o ndcfg $(OFILES) -lefence -lx -lhpsl -lresmgr -lcurses -lelf -lcmd -lc
# "dmalloc" debugging malloc package:
#	$(CC) -g -o ndcfg $(OFILES) -ldmalloc -lx -lhpsl -lresmgr -lcurses -lelf -lcmd -lc

ndisl: clean $(OFILES)
	$(CC) -o ndisl $(OFILES) -lx -lhpsl -lresmgr -lcurses -lelf -lcmd -lc

resops.o: resops.c
	$(CC) $(LOCALCFLAGS) $(CFLAGS) -c resops.c

tmpcmdlex.o: tmpcmdlex.c
	$(CC) $(LOCALCFLAGS) $(CFLAGS) -c tmpcmdlex.c

tmpcmdyacc.o: tmpcmdyacc.c
	$(CC) $(LOCALCFLAGS) $(CFLAGS) -c tmpcmdyacc.c

tmpbcfglex.o: tmpbcfglex.c
	$(CC) $(LOCALCFLAGS) $(CFLAGS) -c tmpbcfglex.c

tmpbcfgyacc.o: tmpbcfgyacc.c
	$(CC) $(LOCALCFLAGS) $(CFLAGS) -c tmpbcfgyacc.c

# Since we have two lexers, we must substitute variable names in one of them.
tmpbcfglex.c: bcfglex.l common.h
	$(LEX) -t bcfglex.l | sed -e 's/yy/zz/g' -e 's/YY/ZZ/g' > tmpbcfglex.c

# Since we have two lexers, we must substitute variable names in one of them.
tmpbcfgyacc.c: bcfgparser.y common.h
	$(YACC) -d bcfgparser.y
	sed -e 's/yy/zz/g' -e 's/YY/ZZ/g' y.tab.h > tmpbcfgtab.h
	sed -e 's/yy/zz/g' -e 's/YY/ZZ/g' y.tab.c > tmpbcfgyacc.c
	-rm -f y.tab.h y.tab.c

tmpcmdlex.c: cmdlex.l common.h
	$(LEX) -t cmdlex.l > tmpcmdlex.c

tmpcmdyacc.c: cmdparser.y common.h
	$(YACC) -d cmdparser.y
	mv y.tab.h tmpcmdtab.h
	mv y.tab.c tmpcmdyacc.c

bcfgops.o: bcfgops.c common.h
	$(CC) $(LOCALCFLAGS) $(CFLAGS) -c bcfgops.c

cmdops.o: cmdops.c common.h
	$(CC) $(LOCALCFLAGS) $(CFLAGS) -c cmdops.c

ca.o: ca.c common.h
	$(CC) $(LOCALCFLAGS) $(CFLAGS) -c ca.c

elfops.o: elfops.c common.h
	$(CC) $(LOCALCFLAGS) $(CFLAGS) -c elfops.c

main.o: main.c common.h
	$(CC) $(LOCALCFLAGS) $(CFLAGS) -c main.c

tags:
	ctags -a *.c

install: all

clean:
	-rm -f tmpbcfgtab.h tmpcmdtab.h tmpcmdyacc.c tmpcmdyacc.o tmpcmdlex.c tmpcmdlex.o y.tab.h y.tab.c core.* y.tab.c tmpbcfglex.c tmpbcfglex.o tmpbcfgyacc.c tmpbcfgyacc.o bcfgops.o main.o resops.o y.tab.h cmdops.o ca.o elfops.o tags

clobber: clean
	-rm -f $(TARGET)
