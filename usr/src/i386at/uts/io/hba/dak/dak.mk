#ident	"@(#)kern-i386at:io/hba/dak/dak.mk	1.2.2.3"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	dak.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/dak.mk

DAK = dak.cf/Driver.o
BINARIES  =$(DAK)
LFILE = $(LINTDIR)/dak.ln


ASFLAGS = -m

FILES = dak.o
CFILES = dak.c
LFILES = dak.ln
PROBEFILE=dak.c

SRCFILES = $(CFILES)

all:
	-@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DAK) $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
		if [ ! -r $(DAK) ]; then \
			echo "ERROR: $(DAK) is missing" 1>&2; \
			false; \
		fi \
	fi


install:	all
		( \
                cd dak.cf ; $(IDINSTALL) -R$(CONF) -M dak; \
		rm -f  $(CONF)/pack.d/dak/disk.cfg;  \
		cp disk.cfg $(CONF)/pack.d/dak/ \
		)


$(DAK):	$(FILES)
		$(LD) -r -o $(DAK) $(FILES)


clean:
	-rm -f *.o $(LFILES) *.L $(DAK)
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $ (BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e dak 

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
	dak.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
