#	Copyright	"%c%"

#ident	"@(#)makefile	1.2"

include $(CMDRULES)

################################################################################

SOURCES = ../zic.c ../scheck.c ../ialloc.c
LSOURCES = zic.c scheck.c ialloc.c
LOBJECTS = zic.o scheck.o ialloc.o

# we want to have zic exec the yearistype which is one level up,
# not the one that may be on the native system
LPATH=..:$(PATH)

FILES = ../asia ../australasia ../europe ../etcetera ../northamerica \
	../pacificnew ../southamerica

all: zic

zic: $(SOURCES)
	sed -e 's/TZDIR/"."/' ../zic.c >zic.c
	cp ../scheck.c ../ialloc.c .
	$(HCC) -o zic $(LSOURCES)

install: all
	for i in $(FILES); do \
	    PATH=$(LPATH) ./zic $$i; done
	for i in `ls -d [A-Z]*`; do \
	    if [ -d $$i ] ; then \
		[ -d $(ETC)/TZ/$$i ] || \
		    mkdir -p $(ETC)/TZ/$$i; \
		for j in `ls $$i`; do \
		    $(INS) -f $(ETC)/TZ/$$i -m 644 $$i/$$j; done; \
	    else \
		$(INS) -f $(ETC)/TZ -m 644 $$i; \
	    fi; \
	done

clean:
	rm -f $(LSOURCES) $(LOBJECTS)

clobber: clean
	rm -rf zic [A-Z]*

lintit:

