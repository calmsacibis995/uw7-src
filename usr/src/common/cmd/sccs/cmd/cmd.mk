#ident	"@(#)sccs:cmd/cmd.mk	6.16.5.6"
#
#

include $(CMDRULES)

HDR = ../hdr


LINT_LIBS = ../lib/llib-lcom.ln \
	../lib/llib-lcassi.ln \
	../lib/llib-lmpw.ln

LIBS = ../lib/comobj.a \
	../lib/cassi.a \
	../lib/mpwlib.a

HELPLOC = $(CCSLIB)/help
UW_HELPLOC = $(UW_CCSLIB)/help
OSR5_HELPLOC = $(OSR5_CCSLIB)/help

LINT_FILES = admin.out	\
	comb.out	\
	delta.out	\
	get.out		\
	prs.out		\
	unget.out	\
	val.out		\
	what.out

C_CMDS = admin	\
	cdc	\
	comb	\
	delta	\
	get	\
	prs	\
	rmdel	\
	sact	\
	unget	\
	val	\
	what

SGSBASE=../..
INC=
INCSYS=
INCLIST=-I$(SGSBASE)/sgs/inc/common

CMDS = $(C_CMDS)	\
	sccsdiff

all:	$(CMDS) help help2 compat_help compat_help2 

$(CMDS): $(LIBS)

admin:	admin.o	$(LIBS)
	$(CC) $(LDFLAGS) admin.o $(LINK_MODE) $(LIBS) -lgen -o admin

admin.o:	admin.c
	$(CC) -c $(INCLIST) $(CFLAGS) admin.c

cdc:	rmchg
	-ln	rmchg cdc

comb:	comb.o	$(LIBS)
	$(CC) $(LDFLAGS) comb.o $(LINK_MODE) $(LIBS) -lgen -o comb

comb.o:	comb.c
	$(CC) -c $(INCLIST) $(CFLAGS) comb.c

delta:	delta.o	$(LIBS)
	$(CC) $(LDFLAGS) delta.o $(LINK_MODE) $(LIBS) -lgen -o delta

delta.o:	delta.c
	$(CC) -c $(INCLIST) $(CFLAGS) delta.c

get:	get.o	$(LIBS)
	$(CC) $(LDFLAGS) get.o $(LINK_MODE) $(LIBS) -lgen -o get

get.o:	get.c
	$(CC) -c $(INCLIST) $(CFLAGS) get.c

help:	help.o
	$(CC) $(LDFLAGS) help.o -o help

help.o:	help.c
	$(CC) -c $(INCLIST) $(CFLAGS) help.c

help2:	help2.o	$(LIBS)
	$(CC) $(LDFLAGS) help2.o $(LINK_MODE) $(LIBS) -lgen -o help2

help2.o:	help2.c
	$(CC) -c $(INCLIST) $(CFLAGS) help2.c

compat_help2:	compat_help2.o	$(LIBS)
	$(CC) $(LDFLAGS) compat_help2.o $(LINK_MODE) $(LIBS) -lgen -o compat_help2

compat_help2.o: help2.c
	$(CC) -c -Wa,"-ocompat_help2.o" $(INCLIST) $(CFLAGS) \
	-DDEFAULT_HELPDIR=\"$(ALT_PREFIX)/usr/ccs/lib/help/\" help2.c

compat_help:	compat_help.o
	$(CC) $(LDFLAGS) compat_help.o -o compat_help

compat_help.o: help.c
	$(CC) -c -Wa,"-ocompat_help.o" $(INCLIST) $(CFLAGS) \
	-DDEFAULT_HELPDIR=\"$(ALT_PREFIX)/usr/ccs/lib/help/\" help.c

prs:	prs.o	$(LIBS)
	$(CC) $(LDFLAGS) prs.o $(LINK_MODE) $(LIBS) -lgen -o prs

prs.o:	prs.c
	$(CC) -c $(INCLIST) $(CFLAGS) prs.c
	
rmdel:	rmchg $(LIBS)
	-ln rmchg rmdel

rmchg:	rmchg.o $(LIBS)
	$(CC) $(LDFLAGS) rmchg.o $(LINK_MODE) $(LIBS) -lgen -o rmchg

rmchg.o:	rmchg.c
	$(CC) -c $(INCLIST) $(CFLAGS) rmchg.c

sact:	unget
	-ln unget sact

sccsdiff:	sccsdiff.sh
	cp sccsdiff.sh sccsdiff
	chmod +x sccsdiff

unget:	unget.o	$(LIBS)
	$(CC) $(LDFLAGS) unget.o $(LINK_MODE) $(LIBS) -lgen -o unget

unget.o:	unget.c
	$(CC) -c $(INCLIST) $(CFLAGS) unget.c

val:	val.o	$(LIBS)
	$(CC) $(LDFLAGS) val.o $(LINK_MODE) $(LIBS) -lgen -o val

val.o:	val.c
	$(CC) -c $(INCLIST) $(CFLAGS) val.c

what:	what.o	$(LIBS)
	$(CC) $(LDFLAGS) what.o $(LINK_MODE) $(LIBS) -lgen -o what

what.o:	what.c
	$(CC) -c $(INCLIST) $(CFLAGS) what.c

$(LIBS):
	cd ../lib; $(MAKE) -f lib.mk

install:	all
	$(STRIP) $(C_CMDS)
	$(STRIP) help help2 compat_help compat_help2
	$(CH)-chmod 775 $(CMDS) help help2 compat_help compat_help2
	$(CH)-chgrp $(GRP) $(CMDS) help2 compat_help compat_help2
	$(CH)-chown $(OWN) $(CMDS) help2 compat_help compat_help2
	-cp $(CMDS) $(CCSBIN)
	-cp help $(CCSBIN)
	-cp $(CMDS) $(UW_CCSBIN)
	-cp compat_help $(UW_CCSBIN)/help
	-cp $(CMDS) $(OSR5_CCSBIN)
	-cp compat_help $(OSR5_CCSBIN)/help
	if [ ! -d $(HELPLOC) ] ; then mkdir $(HELPLOC) ; fi
	if [ ! -d $(HELPLOC)/lib ] ; then mkdir $(HELPLOC)/lib ; fi
	-mv help2 $(HELPLOC)/lib
	if [ ! -d $(UW_HELPLOC) ] ; then mkdir $(UW_HELPLOC) ; fi
	if [ ! -d $(UW_HELPLOC)/lib ] ; then mkdir $(UW_HELPLOC)/lib ; fi
	if [ ! -d $(OSR5_HELPLOC) ] ; then mkdir $(OSR5_HELPLOC) ; fi
	if [ ! -d $(OSR5_HELPLOC)/lib ] ; then mkdir $(OSR5_HELPLOC)/lib ; fi
	-cp compat_help2 $(UW_HELPLOC)/lib/help2
	-cp compat_help2 $(OSR5_HELPLOC)/lib/help2

clean:
	-rm -f *.o
	-rm -f $(LINT_FILES)
	-rm -f rmchg

clobber:	clean
	-rm -f $(CMDS) help help2 compat_help compat_help2

.SUFFIXES : .o .c .e .r .f .y .yr .ye .l .s .out

.c.out:
	rm -f $*.out
	$(LINT) $< $(LINTFLAGS) $(LINT_LIBS) > $*.out

lintit:	$(LINT_FILES)
	@echo "Library $(LLIBRARY) is up to date\n"

