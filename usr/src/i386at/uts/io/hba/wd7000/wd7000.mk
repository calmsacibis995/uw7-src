#ident	"@(#)kern-i386at:io/hba/wd7000/wd7000.mk	1.5"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	wd7000.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/wd7000

WD7000 = wd7000.cf/Driver.o
LFILE = $(LINTDIR)/wd7000.ln

ASFLAGS = -m

FILES = wd7000.o
CFILES = wd7000.c
LFILES = wd7000.ln

SRCFILES = $(CFILES)

.s.o:
	$(AS) -m $<

all:	$(WD7000) 

install:	all
		( \
		cd wd7000.cf ; $(IDINSTALL) -R$(CONF) -M wd7000; \
		rm -f $(CONF)/pack.d/wd7000/disk.cfg;  \
		cp disk.cfg $(CONF)/pack.d/wd7000/  \
		)

$(WD7000):	$(FILES)
		$(LD) -r -o $(WD7000) $(FILES)


clean:
	-rm -f *.o $(LFILES) *.L $(WD7000)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e wd7000

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
          had.h \
          wd7000.h


headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
