#ident	"@(#)kern-i386at:io/layer/vtoc/vtoc.mk	1.1.2.2"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE= vtoc.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/target/vtoc

VTOC = vtoc.cf/Driver.o
LFILE = $(LINTDIR)/sd01.ln

FILES = vtoc.o
CFILES = vtoc.c
LFILES = vtoc.ln

SRCFILES = $(CFILES)

all:	$(VTOC)

install:	all
		(cd vtoc.cf ; $(IDINSTALL) -R$(CONF) -M vtoc)

$(VTOC):	$(FILES)
		$(LD) -r -o $(VTOC) $(FILES)


clean:
	-rm -f *.o $(LFILES) *.L $(VTOC)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e vtoc

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

sysHeaders = vtocos5.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
