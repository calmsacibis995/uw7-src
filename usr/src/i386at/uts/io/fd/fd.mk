#ident	"@(#)fd.mk	1.11"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	fd.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/fd

FD = fd.cf/Driver.o
FDBUF = fdbuf.cf/Driver.o
LFILE = $(LINTDIR)/fd.ln

FILES =  \
	fd.o  \
	fdbuf.o

CFILES =  \
	fd.c  \
	fdbuf.c

LFILES =  \
	fd.ln  \
	fdbuf.ln

all:	$(FD) $(FDBUF)

install: all
	cd fd.cf; $(IDINSTALL) -R$(CONF) -M fd
	cd fdbuf.cf; $(IDINSTALL) -R$(CONF) -M fdbuf

$(FD):	fd.o
	$(LD) -r -o $@ fd.o

$(FDBUF):	fdbuf.o
	$(LD) -r -o $@ fdbuf.o

#
# Configuration Section
#


clean:
	-rm -f $(FILES) $(FD) $(FDBUF)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d fd
	-$(IDINSTALL) -R$(CONF) -e -d fdbuf
	-rm -f $(LFILES) *.L

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE):	$(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(CFILES); do \
		echo $$i; \
	done


FRC:

#
# Header Install Section
#

sysHeaders = \
	fd.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done


include $(UTSDEPEND)

include $(MAKEFILE).dep
