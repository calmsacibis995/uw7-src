#ident	"@(#)char.mk	1.8"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	char.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/char

CHAR = char.cf/Driver.o
LFILE = $(LINTDIR)/char.ln

FILES = \
	char.o

CFILES = \
	char.c

LFILES = \
	char.ln

all:	$(CHAR)

install: all
	(cd char.cf; $(IDINSTALL) -R$(CONF) -M char)

$(CHAR): $(FILES)
	$(LD) -r -o $(CHAR) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(CHAR)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e char

$(LINTDIR):
	-mkdir -p $@

lintit: $(LFILE)

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


#
# Header Install Section
#
sysHeaders = \
	char.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
