#ident  "@(#)osm.mk	1.12"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	osm.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/i2o/osm
HBADIR = $(KBASE)/io/hba

OSM= osm.cf/Driver.o
PROBEFILE = i2o_osm.c
PROBEFILE2 = lanosm.c
LANOSM = lanosm.cf/Driver.o

LFILE = $(LINTDIR)/osm.ln

UWINC = ./inc
I2OINC = ../inc/i2o
MAININC = ../inc
COMINC = ../inc/com
LOCALINC =  -I$(UWINC) -I$(I2OINC) -I$(MAININC) -I$(COMINC)
LOCALDEF = -DMAD

FILES = i2o_osm.o osmutil.o
LANFILES = lanosm.o
CFILES = i2o_osm.c osmutil.c lanosm.c
LFILES = i2o_osm.ln osmutil.ln lanosm.ln

SRCFILES = $(CFILES)

all:
	-@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(OSM) $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
		if [ ! -r $(OSM) ]; then \
			echo "ERROR: $(OSM) is missing" 1>&2; \
			false; \
		fi \
	fi
	-@if [ -f $(PROBEFILE2) ]; then \
		$(MAKE) -f $(MAKEFILE) $(LANOSM) $(MAKEARGS) "KBASE=$(KBASE)" \
				"LOCALDEF=$(LOCALDEF)" ;\
	else \
		if [ ! -r $(LANOSM) ]; then \
			echo "ERROR: $(LANOSM) is missing" 1>&2; \
			false; \
		fi \
	fi

install:	all
		(cd osm.cf ; $(IDINSTALL) -R$(CONF) -M i2oOSM; \
		rm -f $(CONF)/pack.d/i2oOSM/disk.cfg;	\
		cp disk.cfg $(CONF)/pack.d/i2oOSM;	\
		rm -rf ../$(HBADIR)/i2oOSM/i2oOSM.cf;	\
		mkdir -p ../$(HBADIR)/i2oOSM/i2oOSM.cf;	\
		cp Drvmap Driver.o System Master Space.c disk.cfg \
			../$(HBADIR)/i2oOSM/i2oOSM.cf;	)
		cd lanosm.cf ; $(IDINSTALL) -R$(CONF) -M lanosm

$(OSM):	$(FILES)
		$(LD) -r -o $(OSM) $(FILES)

$(LANOSM): $(LANFILES)
		$(LD) -r -o $(LANOSM) $(LANFILES)

clean:
	-rm -f *.o $(LFILES) *.L $(OSM) $(LANOSM)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e i2oOSM
	rm -rf $(HBADIR)/i2oOSM
	$(IDINSTALL) -R$(CONF) -d -e lanosm

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

sysHeaders = 

headinstall:

#include $(UTSDEPEND)

include $(MAKEFILE).dep
