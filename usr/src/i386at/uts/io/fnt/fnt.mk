#ident	"@(#)fnt.mk	1.2"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	fnt.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/fnt

FNT = fnt.cf/Driver.o
LFILE = $(LINTDIR)/kd.ln

FILES = \
	codeset0.o \
	fntdrv.o \
	fntwrap.o

CFILES = \
	fntdrv.c \
	fntwrap.c

LFILES = \
	fntdrv.ln \
	fntwrap.ln

all: $(FNT)

fntdrv.o: fntdrv.c fnt.h
fntwrap.o: fntwrap.c fnt.h

install: all
	(cd fnt.cf; $(IDINSTALL) -R$(CONF) -M fnt)

$(FNT): $(FILES)
	$(LD) -r -o $(FNT) $(FILES)

# include codeset0.c directly, to avoid need for bdftofnt
#
# codeset0.c: bdftofnt/bdftofnt
#	bdftofnt/bdftofnt 8x16rk.bdf > codeset0.c
#
# bdftofnt/bdftofnt:
# 	cd bdftofnt; make -f bdftofnt.mk bdftofnt

clean:
	-rm -f *.o $(LFILES) *.L $(FNT)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e fnt 

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
	@for i in $(CFILES); do \
		echo $$i; \
	done

sysHeaders = \
	fnt.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
