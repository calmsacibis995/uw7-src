#ident  "@(#)txport.mk	1.8"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	txport.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/i2o/txport

TXPORT= txport.cf/Driver.o
PROBEFILE = trans.c
LFILE = $(LINTDIR)/txport.ln

I2OINC = ../inc/i2o
MAININC = ../inc
COMINC = ../inc/com
LOCALINC =  -I$(I2OINC) -I$(MAININC) -I$(COMINC)
LOCALDEF = -DMADRONA

FILES = trans.o uwaretx.o
CFILES = trans.c uwaretx.c
LFILES = trans.ln uwaretx.ln

SRCFILES = $(CFILES)

all:
	-@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(TXPORT) $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
		if [ ! -r $(TXPORT) ]; then \
			echo "ERROR: $(TXPORT) is missing" 1>&2; \
			false; \
		fi \
	fi

install:	all
		cd txport.cf ; $(IDINSTALL) -R$(CONF) -M i2otrans

$(TXPORT):	$(FILES)
		$(LD) -r -o $(TXPORT) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(TXPORT)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e i2otrans

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
