#ident  "@(#)msg.mk	1.8"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	msg.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/i2o/msg

MSG= msg.cf/Driver.o
PROBEFILE = osmmsg.c
LFILE = $(LINTDIR)/msg.ln

UWINC = ./inc
I2OINC = ../inc/i2o
MAININC = ../inc
COMINC = ../inc/com
LOCALINC =  -I$(UWINC) -I$(I2OINC) -I$(MAININC) -I$(COMINC)
LOCALDEF = -DMAD

FILES = osmmsg.o uwaremsg.o
CFILES = osmmsg.c uwaremsg.c
LFILES = osmmsg.ln uwaremsg.ln

SRCFILES = $(CFILES)

all:
	-@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(MSG) $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
		if [ ! -r $(MSG) ]; then \
			echo "ERROR: $(MSG) is missing" 1>&2; \
			false; \
		fi \
	fi

install:	all
		cd msg.cf ; $(IDINSTALL) -R$(CONF) -M i2omsg

$(MSG):	$(FILES)
		$(LD) -r -o $(MSG) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(MSG)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e i2omsg

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
