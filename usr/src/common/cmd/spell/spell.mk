#ident	"@(#)spell:spell.mk	1.18.1.6"
#ident	"$Header$"


include $(CMDRULES)

#	spell make file
#	Note:  In using the -f flag it is assumed that either
#	both the host and the target machines need the -f, or
#	neither needs it.  If one needs it and the other does
#	not, it is assumed that the machine that does not need
#	it will treat it appropriately.

OWN = bin
GRP = bin

LIBDIR   = $(USRLIB)/spell
SHAREDIR = $(USRSHARE)/lib/spell
DIRS     = $(LIBDIR) $(SHAREDIR)

MAINS = spellin spellprog hashmake hashcheck spellin1 hashmk1 \
		spell hlista hlistb hstop compress spellhist

all: $(MAINS)

spell: spellprog spell.sh
	cp spell.sh spell

compress: compress.sh
	cp compress.sh compress

spellprog: spellprog.o hash.o hashlook.o huff.o malloc.o
	$(CC) -o $@ spellprog.o hash.o hashlook.o huff.o malloc.o \
		$(LDFLAGS) $(LDLIBS) $(PERFLIBS)

spellin: spellin.o huff.o
	$(CC) -o $@ spellin.o huff.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

spellin1: spellin1.o huff1.o
	$(HCC) -o $@ spellin1.o huff1.o 

spellhist:
	echo '\c' > spellhist

hashcheck: hashcheck.o hash.o huff.o
	$(CC) -o $@ hashcheck.o hash.o huff.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

hashmk1: hashmake1.o hash1.o
	$(HCC) -o $@ hashmake1.o hash1.o 

hashmake: hashmake.o hash.o
	$(CC) -o $@ hashmake.o hash.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

hash.o: hash.c \
	hash.h

hash1.o: hash.c \
	hash.h
	cp hash.c hash1.c
	$(HCC) -c hash1.c
	rm -f hash1.c

hashcheck.o: hashcheck.c \
	$(INC)/stdio.h \
	hash.h

hashlook.o: hashlook.c \
	$(INC)/stdio.h \
	hash.h \
	huff.h

hashmake.o: hashmake.c \
	$(INC)/stdio.h \
	hash.h

hashmake1.o: hashmake.c \
	$(INC)/stdio.h \
	hash.h
	cp hashmake.c hashmake1.c
	$(HCC) -c hashmake1.c
	rm -f hashmake1.c

huff.o: huff.c \
	$(INC)/stdio.h \
	huff.h

huff1.o: huff.c \
	$(INC)/stdio.h \
	huff.h
	cp huff.c huff1.c
	$(HCC) -c huff1.c
	rm -f huff1.c

malloc.o: malloc.c

spellin.o: spellin.c \
	$(INC)/stdio.h \
	hash.h

spellin1.o: spellin.c \
	$(INC)/stdio.h \
	hash.h
	cp spellin.c spellin1.c
	$(HCC) -c spellin1.c
	rm -f spellin1.c

spellprog.o: spellprog.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h

alldata: hlista hlistb hstop
	-rm -f htemp1

htemp1: list local extra hashmk1
	cat list local extra | $(_SH_) ./hashmk1 >htemp1

hlista: american hashmake hashmk1 spellin spellin1 htemp1
	$(_SH_) ./hashmk1 <american |sort -u - htemp1 >htemp2
	$(_SH_) ./spellin1 `wc htemp2|sed -n 's/\([^ ]\) .*/\1/p' ` <htemp2 >hlista
	-rm -f htemp2

hlistb: british hashmk1 spellin1 htemp1
	$(_SH_) ./hashmk1 <british |sort -u - htemp1 >htemp3
	$(_SH_) ./spellin1 `wc htemp3|sed -n 's/\([^ ]\) .*/\1/p' ` <htemp3 >hlistb
	-rm -f htemp3

hstop: stop spellin1 hashmk1
	$(_SH_) ./hashmk1 <stop | sort -u >htemp4
	$(_SH_) ./spellin1 `wc htemp4|sed -n 's/\([^ ]\) .*/\1/p' ` <htemp4 >hstop
	-rm -f htemp4

install: all uxspell.str $(DIRS)
	-rm -f $(LIBDIR)/hstop
	-rm -f $(LIBDIR)/hlistb
	-rm -f $(LIBDIR)/hlista
	-rm -f $(LIBDIR)/compress
	$(INS) -f $(SHAREDIR) -m 0644 -u $(OWN) -g $(GRP) hstop
	$(INS) -f $(SHAREDIR) -m 0644 -u $(OWN) -g $(GRP) hlistb
	$(INS) -f $(SHAREDIR) -m 0644 -u $(OWN) -g $(GRP) hlista
	$(INS) -f $(SHAREDIR) -m 0555 -u $(OWN) -g $(GRP) compress
	$(INS) -f $(LIBDIR) -m 0555 -u $(OWN) -g $(GRP) spellprog
	$(INS) -f $(LIBDIR) -m 0555 -u $(OWN) -g $(GRP) hashmake
	$(INS) -f $(LIBDIR) -m 0555 -u $(OWN) -g $(GRP) hashcheck
	$(INS) -f $(LIBDIR) -m 0555 -u $(OWN) -g $(GRP) spellin
	$(INS) -f $(VAR)/adm -m 0666 -u $(OWN) -g $(GRP) spellhist
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) spell
	-$(SYMLINK) $(ROOT)/$(MACH)/usr/share/lib/spell/hstop $(LIBDIR)/hstop
	-$(SYMLINK) $(ROOT)/$(MACH)/usr/share/lib/spell/hlistb $(LIBDIR)/hlistb
	-$(SYMLINK) $(ROOT)/$(MACH)/usr/share/lib/spell/hlista $(LIBDIR)/hlista
	-$(SYMLINK) $(ROOT)/$(MACH)/usr/share/lib/spell/compress $(LIBDIR)/compress
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES uxspell.str

$(DIRS):
	[ -d $@ ] || mkdir -p $@
	$(CH)chmod 555 $@
	$(CH)chgrp $(GRP) $@
	$(CH)chown $(OWN) $@

clean:
	-rm -f *.o

clobber: clean 
	-rm -f $(MAINS)
	-rm -f htemp1 htemp2 htemp3 htemp4

lintit:
	$(LINT) $(LINTFLAGS) spellprog.c hash.c hashlook.c huff.c malloc.c
	$(LINT) $(LINTFLAGS) spellin.c huff.c
	$(LINT) $(LINTFLAGS) hashcheck.c hash.c huff.c
	$(LINT) $(LINTFLAGS) hashmake.c hash.c

# optional targets

BIN = P108
LIST = opr -ttx -b${BIN}
RDIR = $(ROOT)/usr/src/common/cmd/spell
REL  = current
CSID = -r`gsid spellcode ${REL}`
DSID = -r`gsid spelldata ${REL}`
SHSID = -r`gsid spell.sh ${REL}`
CMPRSID = -r`gsid compress.sh ${REL}`
MKSID = -r`gsid spell.mk ${REL}`
SFILES = spellprog.c spellin.c
DFILES = american british local list extra stop

inssh:   ; ${MAKE} -f spell.mk spell
inscomp: ; ${MAKE} -f spell.mk compress
inscode: ; ${MAKE} -f spell.mk spell 
insdata: ; ${MAKE} -f spell.mk alldata

listing:  ; pr spell.mk spell.sh compress.sh ${SFILES} ${DFILES} | ${LIST}
listmk:   ; pr spell.mk | ${LIST}
listsh:   ; pr spell.sh | ${LIST}
listcomp: ; pr compress.sh | ${LIST}
listcode: ; pr ${SFILES} | ${LIST}
listdata: ; pr ${DFILES} | ${LIST}

build: bldmk bldsh bldcomp bldcode blddata
	:
bldcode: ; get -p ${CSID} s.spell.src ${REWIRE} | ntar -d ${RDIR} -g
blddata: ; get -p ${DSID} s.spell.data | ntar -d ${RDIR} -g
bldsh:   ; get -p ${SHSID} s.spell.sh ${REWIRE} > ${RDIR}/spell.sh
bldcomp: ; get -p ${CMPRSID} s.compress.sh ${REWIRE} > ${RDIR}/compress.sh
bldmk:   ; get -p ${MKSID} s.spell.mk > ${RDIR}/spell.mk

edit: sedit dedit mkedit shedit compedit
	:
sedit:   ; get -p -e s.spell.src | ntar -g
dedit:   ; get -p -e s.spell.data | ntar -g
shedit:  ; get -e s.spell.sh
compedit: ; get -e s.compress.sh

delta: sdelta ddelta mkdelta shdelta compdelta
	:
sdelta:
	ntar -p ${SFILES} > spell.src
	delta s.spell.src
	-rm -f ${SFILES}
ddelta:
	ntar -p ${DFILES} > spell.data
	delta s.spell.data
	-rm -f ${DFILES}
shdelta:
	delta s.spell.sh
compdelta: ; delta s.compress.sh

mkedit: ; get -e s.spell.mk
mkdelta: ; delta s.spell.mk

delete: clobber shdelete compdelete
	-rm -f ${SFILES} ${DFILES}

shdelete: shclobber
	-rm -f spell.sh

compdelete: compclobber
	-rm -f compress.sh

shclobber:  ; -rm -f spell
compclobber:; -rm -f compress
