#ident	"@(#)sum.mk	1.2"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	sum.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = acc/priv/sum

SUM = sum.cf/Driver.o
LFILE = $(LINTDIR)/sum.ln

FILES = \
	sum.o

CFILES = \
	sum.c

SRCFILES = $(CFILES)

LFILES = \
	sum.ln

all:	$(SUM)

install: all
	(cd sum.cf; $(IDINSTALL) -R$(CONF) -M sum)

$(SUM): $(FILES)
	$(LD) -r -o $(SUM) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(SUM)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e sum
	
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

headinstall: \
	$(KBASE)/acc/priv/sum/sum.h 
	$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $(KBASE)/acc/priv/sum/sum.h

include $(UTSDEPEND)

include $(MAKEFILE).dep
