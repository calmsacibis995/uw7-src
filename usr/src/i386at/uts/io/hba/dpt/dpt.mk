#ident	"@(#)kern-i386at:io/hba/dpt/dpt.mk	1.7.3.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	dpt.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/dpt

DPT = dpt.cf/Driver.o
LFILE = $(LINTDIR)/dpt.ln

LOCALDEF = -D_DDI=8 -DDPT_TARGET_MODE -DDPT_TIMEOUT_RESET_SUPPORT 
RAIDDEF = $(LOCALDEF) -DDPT_RAID_0

ASFLAGS = -m

FILES = dpt.o
CFILES = dpt.c
LFILES = dpt.ln

CHECK_RAID_FILE = dpt_raid.c
RAIDFILES = dpt.o dpt_raid_osd.o dpt_raid.o
RAIDCFILES = dpt.c dpt_raid_osd.c dpt_raid.c
RAIDLFILES = dpt.ln dpt_raid_osd.ln dpt_raid.ln

SRCFILES = $(CFILES)

all:
	@if [ -f $(CHECK_RAID_FILE) ]; then \
		$(MAKE) -f $(MAKEFILE) dptraid $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(RAIDDEF)" ; \
	else \
		$(MAKE) -f $(MAKEFILE) dpt $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ; \
	fi

install:	all
		( \
		cd dpt.cf ; $(IDINSTALL) -R$(CONF) -M dpt; \
		rm -f  $(CONF)/pack.d/dpt/disk.cfg;  \
		cp disk.cfg $(CONF)/pack.d/dpt/ \
		)


dptraid: $(RAIDFILES)
		$(LD) -r -o $(DPT) $(RAIDFILES)
#		$(FUR) -W -o dpt.order $(DPT)

dpt:	$(FILES)
		$(LD) -r -o $(DPT) $(FILES)
#		$(FUR) -W -o dpt.order $(DPT)

clean:
	-rm -f *.o $(LFILES) *.L $(DPT)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e dpt

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
	dpt.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
