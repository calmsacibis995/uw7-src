#ident	"@(#)dma.mk	1.8"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	dma.mk
KBASE = ../..
DIR = io/dma
DMA = dma.cf/Driver.o

CFILES = dma.c

SRCFILES = $(CFILES)

DFILES = \
	dma.o \
	i8237A.o

LINTDIR = $(KBASE)/lintdir

LFILE = $(LINTDIR)/dma.ln

LFILES = \
	dma.ln \
	i8237A.ln

all:		$(DMA)

install:	all
	cd dma.cf; $(IDINSTALL) -R$(CONF) -M dma

$(DMA):	$(DFILES)
	$(LD) -r -o $@ $(DFILES)


clean:
	-rm -f  *.o $(DMA)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d dma
	-rm -f $(LFILES) *.L

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE) FRC

$(LFILE):	$(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

FRC:

#
# Header Install Section
#
headinstall:


include $(UTSDEPEND)

include $(MAKEFILE).dep
