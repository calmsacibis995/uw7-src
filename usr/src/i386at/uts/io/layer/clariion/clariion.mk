#ident	"@(#)kern-i386at:io/layer/clariion/clariion.mk	1.1.2.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	clariion.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/target/clariion

CLMP = clariion.cf/Driver.o
LFILE = $(LINTDIR)/clariion.ln

FILES = clariion.o clariion_qm.o

CFILES = clariion.c clariion_qm.c

LFILES = clariion.ln clariion_qm.ln

SRCFILES = $(CFILES)

all:	$(CLMP)

install:	all
		(cd clariion.cf ; $(IDINSTALL) -R$(CONF) -M clariion)

$(CLMP):	$(FILES)
		$(LD) -r -o $(CLMP) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(CLMP)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e clariion

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

headinstall:

include $(UTSDEPEND)

include $(MAKEFILE).dep
