#ident	"@(#)kern-i386at:io/hba/adss/adss.mk	1.4"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE = adss.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/adss
LOCALDEF = -DUSL_UNIX

ADSS = adss.cf/Driver.o
LFILE = $(LINTDIR)/adss.ln

FILES = \
	adss.o \
	him_code/him6x60.o

CFILES = \
	adss.c

HFILES = \
	him_code/him6x60.c

SFILES =

LFILES = adss.ln

SRCFILES = $(CFILES) $(HFILES) $(SFILES)

all:	$(ADSS)

install:	all
		(cd adss.cf ; $(IDINSTALL) -R$(CONF) -M adss; \
		rm -f $(CONF)/pack.d/adss/disk.cfg;	\
		cp disk.cfg $(CONF)/pack.d/adss	)

$(ADSS):	$(FILES)
		$(LD) -r -o $(ADSS) $(FILES)

clean:
	-rm -f *.o him_code/*.o $(LFILES) *.L $(ADSS)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e adss

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(HFILES:.c=.o):
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<
	-mv `expr $@ : 'him_code/\(.*\)'` $@

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
	adss.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep

him_code/him6x60.o: \
	him_code/him6x60.c \
	../../../io/hba/adss/him_code/aic6x60.h \
	../../../io/hba/adss/him_code/scb6x60.h \
	../../../io/hba/adss/him_code/him_scsi.h \
	$(FRC)
