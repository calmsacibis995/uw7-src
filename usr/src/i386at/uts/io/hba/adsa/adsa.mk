#ident	"@(#)kern-i386at:io/hba/adsa/adsa.mk	1.7"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	adsa.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/adsa
#LOCALDEF = -DUNIPROC
#LOCALDEF = -U_LOCKTEST -DAHA_DEBUG2
#LOCALDEF = -U_LOCKTEST -DAHA_DEBUG_TRACE

ADSA = adsa.cf/Driver.o
LFILE = $(LINTDIR)/adsa.ln

FILES = \
	adsa.o \
	him_code/him.o \
	him_code/him_init.o

CFILES = \
	adsa.c

HFILES = \
	him_code/him.c \
	him_code/him_init.c

SFILES =

LFILES = adsa.ln

SRCFILES = $(CFILES) $(HFILES) $(SFILES)

all:	$(ADSA)

install:	all
		(cd adsa.cf ; $(IDINSTALL) -R$(CONF) -M adsa; \
		rm -f $(CONF)/pack.d/adsa/disk.cfg;	\
		cp disk.cfg $(CONF)/pack.d/adsa	)

$(ADSA):	$(FILES)
		$(LD) -r -o $(ADSA) $(FILES)

clean:
	-rm -f *.o him_code/*.o $(LFILES) *.L $(ADSA)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e adsa

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(HFILES:.c=.o):
	$(CC) -DUSL -DSVR40 $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<
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
	adsa.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep

him_code/him.o: \
	him_code/him.c \
	../../../io/hba/adsa/him_code/him_equ.h \
	../../../io/hba/adsa/him_code/him_scb.h \
	../../../io/hba/adsa/him_code/seq_off.h \
	$(FRC)

him_code/him_init.o: \
	him_code/him_init.c \
	../../../io/hba/adsa/him_code/him_equ.h \
	../../../io/hba/adsa/him_code/him_rel.h \
	../../../io/hba/adsa/him_code/him_scb.h \
	../../../io/hba/adsa/him_code/seq_off.h \
	../../../io/hba/adsa/him_code/sequence.h \
	$(FRC)
