#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)diff3:diff3.mk	1.9.3.1"

include $(CMDRULES)

REL = current
SSID = -r`gsid diff3 $(REL)`
CSID = -r`gsid diff3prog $(REL)`
MKSID = -r`gsid diff3.mk $(REL)`
LIST = lp
INSDIR = $(USRBIN)
OWN = bin
GRP = bin
INSLIB = $(USRLIB)
SHSOURCE = diff3.sh
CSOURCE = diff3prog.c

compile all: diff3 diff3prog

diff3:
	cp diff3.sh diff3

diff3prog:	diff3prog.c
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ diff3prog.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) diff3
	$(INS) -f $(INSLIB) -m 0555 -u $(OWN) -g $(GRP) diff3prog

build:	bldmk blddif3p
	get -p $(SSID) s.diff3.sh $(REWIRE) > $(RDIR)/diff3.sh
blddif3p:
	get -p $(CSID) s.diff3prog.c $(REWIRE) > $(RDIR)/diff3prog.c
bldmk:
	get -p $(MKSID) s.diff3.mk > $(RDIR)/diff3.mk

listing:  ;	pr diff3.mk $(SHSOURCE) $(CSOURCE) | $(LIST)
listdif3: ;	pr $(SHSOURCE) | $(LIST)
lsitdif3p: ;	pr $(CSOURCE) | $(LIST)
listmk: ;	pr diff3.mk | $(LIST)

edit:
	get -e s.diff3.sh
dif3pedit:
	get -e s.diff3prog.c

delta:
	delta s.diff3.sh
dif3pdelta:
	delta s.diff3prog.c

mkedit:  ;  get -e s.diff3.mk
mkdelta: ;  delta s.diff3.mk

clean:
	:

clobber: clean
	rm -f diff3 diff3prog

delete:	clobber
	rm -f $(SHSOURCE) $(CSOURCE)
