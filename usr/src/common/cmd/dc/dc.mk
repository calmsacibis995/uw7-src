#	copyright	"%c%"

#ident	"@(#)dc:dc.mk	1.11.1.3"
#ident "$Header$"
#	dc make file

include $(CMDRULES)
REL = current
CSID = -r`gsid dc $(REL)`
MKSID = -r`gsid dc.mk $(REL)`
LIST = lp
INSDIR = $(USRBIN)
OWN = bin
GRP = bin
SOURCE = dc.h dc.c
MAKE = make

compile all: dc

dc:	dc.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ dc.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) dc

build:	bldmk
	get -p $(CSID) s.dc.src $(REWIRE) | ntar -d $(RDIR) -g
bldmk:  ;  get -p $(MKSID) s.dc.mk > $(RDIR)/dc.mk

listing:
	pr dc.mk $(SOURCE) | $(LIST)
listmk: ;  pr dc.mk | $(LIST)

edit:
	get -e -p s.dc.src | ntar -g

delta:
	ntar -p $(SOURCE) > dc.src
	delta s.dc.src
	rm -f $(SOURCE)

mkedit:  ;  get -e s.dc.mk
mkdelta: ;  delta s.dc.mk

clean:
	:

clobber:
	  rm -f dc

delete:	clobber
	rm -f $(SOURCE)
