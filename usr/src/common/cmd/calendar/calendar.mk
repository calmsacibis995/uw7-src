#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)calendar:calendar.mk	1.13.3.1"
#	calendar make file

include $(CMDRULES)

REL = current
LIST = lp
PINSDIR = $(USRLIB)
INSDIR = $(USRBIN)
OWN=bin
GRP=bin
SHSOURCE = calendar.sh
PSOURCE = calprog.c
SHFILES = calendar

compile all: calendar calprog
	:

calendar:
	cp calendar.sh calendar

calprog:
	$(CC) $(CFLAGS) $(DEFLIST) $(LDFLAGS) -o calprog calprog.c $(LDLIBS) \
		$(SHLIBS)

install:	all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) calendar
	$(INS) -f $(PINSDIR) -m 0555 -u $(OWN) -g $(GRP) calprog

inscalp:	calprog
	$(INS) -f $(PINSDIR) calprog

build:	bldmk bldcalp
	get -p -r`gsid calendar $(REL)` s.calendar.sh $(REWIRE) > $(RDIR)/calendar.sh
bldcalp:
	get -p -r`gsid calprog $(REL)` s.calprog.c $(REWIRE) > $(RDIR)/calprog.c
bldmk:  ;  get -p -r`gsid calendar.mk $(REL)` s.calendar.mk > $(RDIR)/calendar.mk

listing:
	pr calendar.mk $(SHSOURCE) $(PSOURCE) | $(LIST)
lstcal: ; pr $(SHSOURCE) | $(LIST)
lstcalp: ; pr $(PSOURCE) | $(LIST)
listmk: ;  pr calendar.mk | $(LIST)

caledit: ; get -e s.calendar.sh
calpedit: ; get -e s.calprog.c

caldelta: ; delta s.calendar.sh
calpdelta: ; delta s.calprog.c

mkedit:  ;  get -e s.calendar.mk
mkdelta: ;  delta s.calendar.mk

clean: ;   :
calclean: ; :
calpclean: ; :

clobber:
	rm -f calendar calprog
calclobber:
	rm -f calendar
calpclobber:
	rm -f calprog

delete:	clobber
	rm -f $(SHSOURCE) $(PSOURCE)
caldelete:	calclobber
	rm -f $(SHSOURCE)
calpdelete:	calpclobber
	rm -f $(PSOURCE)
