#	copyright	"%c%"

#ident	"@(#)Makefile	1.2"
#ident  "$Header$"

include $(CMDRULES)

#
# Makefile for lp/model
#

TOP	=	..

include ../common.mk

MODELDIR= $(USRLIBLP)/model

FMODES = 0444

LOCALINC = -I$(LPINC)

LDLIBS = $(LIBLP) -lgen

SRCS = \
	lp.set.c \
	lp.cat.c \
	lp.tell.c \
	drain.output.c \
	lp.lvlproc.c \
	lp.pr \
	B2.c

#
# Don't use $(SRCS:.c=.o) because it doesn't work on lp.cat.c
#
OBJS = \
	lp.set.o \
	lp.cat.o \
	lp.tell.o \
	drain.output.o \
	lp.lvlproc.o \
	lp.pr.o \
	B2.o

CMDS = \
	lp.set \
	lp.cat \
	lp.tell \
	drain.output \
	lp.lvlproc \
	lp.pr

MODELS = \
	standard \
	PS \
	B2 \
	B2.banntrail \
	B2.job

MISC = \
	alert.proto

all: $(CMDS) $(MODELS)

install: all
	for cmd in $(CMDS); \
	do \
	  $(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(LPBINDIR) $$cmd; \
	done
	for model in $(MODELS); \
	do \
	  $(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(MODELDIR) $$model; \
	done
	for misc in $(MISC); \
	do \
	  $(INS) -m $(FMODES) -u $(OWNER) -g $(GROUP) -f $(LPBINDIR) $$misc; \
	done

strip:

clean:
	$(RM) $(OBJS)

clobber: clean
	$(RM) $(CMDS) B2

lp.set: lp.set.o $(LIBLP)
	$(CC) -o lp.set lp.set.o $(LDFLAGS) $(LDLIBS) $(NOSHLIBS) -lcurses

lp.cat: lp.cat.o $(LIBLP)
	$(CC) -o lp.cat lp.cat.o $(LDFLAGS) $(LDLIBS) $(NOSHLIBS) -lcurses

lp.tell: lp.tell.o $(LIBMSG) $(LIBLP)
	$(CC) -o lp.tell lp.tell.o $(LDFLAGS) $(LIBMSG) $(LDLIBS) $(SHLIBS) \
	 	$(NOSHLIBS) -lcurses

drain.output: drain.output.o $(LIBLP)
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(NOSHLIBS) -lcurses

lp.lvlproc: lp.lvlproc.o $(LIBLP)
	$(CC) -o lp.lvlproc lp.lvlproc.o $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

lp.pr: lp.pr.o $(LIBLP)
	$(CC) -o lp.pr lp.pr.o $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

B2: B2.o $(LIBLP)
	$(CC) -o B2 B2.o $(LDFLAGS) $(LDLIBS) $(NOSHLIBS) -lgen

lintit:
	$(LINT) $(LINTFLAGS) $(CFLAGS) lp.set.c $(LINTLP)
	$(LINT) $(LINTFLAGS) $(CFLAGS) lp.cat.c $(LINTLP)
	$(LINT) $(LINTFLAGS) $(CFLAGS) lp.tell.c $(LINTLP) $(LINTMSG)
	$(LINT) $(LINTFLAGS) $(CFLAGS) drain.output.c $(LINTLP)
	$(LINT) $(LINTFLAGS) $(CFLAGS) lp.lvlproc.c $(LINTLP)
	$(LINT) $(LINTFLAGS) $(CFLAGS) lp.pr.c $(LINTLP)
	$(LINT) $(LINTFLAGS) $(CFLAGS) B2.c $(LINTLP)

lintlib:
