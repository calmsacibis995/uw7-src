#ident  "@(#)ptosm.mk	1.2"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	ptosm.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/i2o/ptosm

OSM= ptosm.cf/Driver.o
PROBEFILE = ads_doc.h

LFILE = $(LINTDIR)/ptosm.ln

UWINC = ./inc
I2OINC = ../inc/i2o
MAININC = ../inc
COMINC = ../inc/com
LOCALINC =  -I$(UWINC) -I$(I2OINC) -I$(MAININC) -I$(COMINC)
LOCALDEF = 

FILES = ptosm.o
CFILES = ptosm.c
LFILES = ptosm.ln

SRCFILES = $(CFILES)

all:
	-@if [ -f $(ROOT)/usr/src/$(WORK)/uts/io/i2o/inc/$(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(OSM) $(MAKEARGS) ;\
	else \
		if [ ! -r $(OSM) ]; then \
			echo "ERROR: $(OSM) is missing" 1>&2 ;\
			false ;\
		fi \
	fi

install:	all
		cd ptosm.cf ; $(IDINSTALL) -R$(CONF) -M i2opt

$(OSM):	$(FILES)
		$(LD) -r -o $(OSM) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(OSM)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e i2opt

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

sysHeaders = \
	ptosm.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

#include $(UTSDEPEND)

include $(MAKEFILE).dep
