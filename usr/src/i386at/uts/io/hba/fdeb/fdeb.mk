#ident	"@(#)kern-i386at:io/hba/fdeb/fdeb.mk	1.2.2.1"

include $(UTSRULES)

MAKEFILE=	fdeb.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/fdeb

# The fdinc and fdcommon directories are under the io/hba/fdsb directory
FDINC = ../fdsb/fdinc
FDCOMMON = ../fdsb/fdcommon
LOCALINC = -I$(FDINC)
LOCALDEF = -DTMCIO

FDEB = fdeb.cf/Driver.o
LFILE = $(LINTDIR)/fdeb.ln

ASFLAGS = -m

COMMONSRC = fdcio.c osdio.c xxxstrat.c
FILES = tmcio.o fdcio.o osdio.o xxxstrat.o
CFILES = tmcio.c $(COMMONSRC)
LFILES = tmcio.ln fdcio.ln osdio.ln xxxstrat.ln

SRCFILES = $(CFILES)

PROBEFILE = tmcio.c
BINARIES = $(FDEB)

.s.o:
	$(AS) -m $<

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) DRIVER $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
              	for fl in $(BINARIES); do \
                        if [ ! -r $$fl ]; then \
                                echo "ERROR: $$fl is missing" 1>&2 ;\
                                false ;\
                                break ;\
                        fi \
		done \
	fi

.MUTEX: linkcommon $(FDEB)

DRIVER: linkcommon $(FDEB)

install:	all
		( \
		cd fdeb.cf ; $(IDINSTALL) -R$(CONF) -M fdeb; \
		rm -f $(CONF)/pack.d/fdeb/disk.cfg;  \
		cp disk.cfg $(CONF)/pack.d/fdeb/  \
		)

linkcommon:
	@for i in $(COMMONSRC); do \
		[ -f $$i ] || ln -s $(FDCOMMON)/$$i . ; \
	done

$(FDEB):	$(FILES)
		$(LD) -r -o $(FDEB) $(FILES)


clean:
	-rm -f *.o $(LFILES) *.L

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e fdeb
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi


$(LINTDIR):
	-mkdir -p $@

lintit:	linkcommon $(LFILE)

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
	fdeb.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
