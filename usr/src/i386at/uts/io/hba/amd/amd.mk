#ident	"@(#)kern-i386at:io/hba/amd/amd.mk	1.1.2.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	amd.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/amd

AMD = amd.cf/Driver.o
LFILE = $(LINTDIR)/amd.ln

ASFLAGS = -m

FILES = amd.o scsiport.o ggmini.o
CFILES = amd.c	scsiport.c ggmini.c
LFILES = amd.ln scsiport.ln ggmini.ln

SRCFILES = $(CFILES)

LOCALDEF= -DNO64BIT -DOTHER_OS -W0,-B

all:
	@if [ -f amd.c ]; then \
                $(MAKE) -f $(MAKEFILE) $(AMD) $(MAKEARGS) "KBASE=$(KBASE)" \
                        "LOCALDEF=$(LOCALDEF)" ;\
        else \
		if [ ! -r $(AMD) ]; then \
			echo "ERROR: $(AMD) is missing" 1>&2 ;\
			false ;\
		fi \
        fi
	

install:	all
		( \
		cd amd.cf ; $(IDINSTALL) -R$(CONF) -M amd; \
		rm -f  $(CONF)/pack.d/amd/disk.cfg;  \
		cp disk.cfg $(CONF)/pack.d/amd/ \
		)


$(AMD):	$(FILES)
		$(LD) -r -o $(AMD) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(AMD)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e amd

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
