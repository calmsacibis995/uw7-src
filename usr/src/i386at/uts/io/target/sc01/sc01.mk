#ident	"@(#)kern-i386at:io/target/sc01/sc01.mk	1.7"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	sc01.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/target/sc01

SC01 = sc01.cf/Driver.o
LFILE = $(LINTDIR)/sc01.ln

FILES = sc01.o
CFILES = sc01.c
LFILES = sc01.ln

SRCFILES = $(CFILES)

all:	$(SC01)

install:	all
		(cd sc01.cf ; $(IDINSTALL) -R$(CONF) -M sc01)

$(SC01):	$(FILES)
		$(LD) -r -o $(SC01) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(SC01)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e sc01

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
	cd_ioctl.h \
	sc01.h 

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
