#ident	"@(#)kern-i386at:io/hba/mcis/mcis.mk	1.5"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	mcis.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/mcis

MCIS = mcis.cf/Driver.o
LFILE = $(LINTDIR)/mcis.ln

ASFLAGS = -m

FILES = mcis.o
CFILES = mcis.c
LFILES = mcis.ln

SRCFILES = $(CFILES)

.s.o:
	$(AS) -m $<

all:	$(MCIS) 

install:	all
		( \
		cd mcis.cf ; $(IDINSTALL) -R$(CONF) -M mcis; \
		rm -f $(CONF)/pack.d/mcis/disk.cfg;  \
		cp disk.cfg $(CONF)/pack.d/mcis/  \
		)

$(MCIS):	$(FILES)
		$(LD) -r -o $(MCIS) $(FILES)


clean:
	-rm -f *.o $(LFILES) *.L $(MCIS)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e mcis

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
          mcis.h


headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
