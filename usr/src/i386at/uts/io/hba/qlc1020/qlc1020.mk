#ident	"@(#)kern-i386at:io/hba/qlc1020/qlc1020.mk	1.2.1.2"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	qlc1020.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/qlc1020

QLC1020= qlc1020.cf/Driver.o
BINARIES  =$(QLC1020)
LFILE = $(LINTDIR)/qlc1020.ln


ASFLAGS = -m

FILES = qlc1020.o
CFILES = qlc1020.c
LFILES = qlc1020.ln
PROBEFILE= qlc1020.c

SRCFILES = $(CFILES)

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(QLC1020) $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
		if [ ! -r $(QLC1020) ]; then \
			echo "ERROR: $(QLC1020) is missing" 1>&2; \
			false; \
		fi \
	fi


install:	all
		( \
                cd qlc1020.cf ; $(IDINSTALL) -R$(CONF) -M qlc1020; \
		rm -f  $(CONF)/pack.d/qlc1020/disk.cfg;  \
		cp disk.cfg $(CONF)/pack.d/qlc1020/ \
		)


$(QLC1020):	$(FILES)
		$(LD) -r -o $(QLC1020) $(FILES)


clean:
	-rm -f *.o $(LFILES) *.L $(QLC1020)
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $ (BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e qlc1020 

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
	qlc1020.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
