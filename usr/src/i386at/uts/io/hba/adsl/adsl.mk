#ident	"@(#)kern-i386at:io/hba/adsl/adsl.mk	1.1.6.1"

include $(UTSRULES)

MAKEFILE = adsl.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/adsl
LOCALDEF = -DUW21 -DUNIXWARE -DCIO

ADSL = adsl.cf/Driver.o
PROBEFILE = adsl.c

LFILES = adsl.ln

LFILE = $(LINTDIR)/$(LFILES)

FILES = \
	adsl.o \
	him_code/hwmdiag.o \
	him_code/hwmdlvr.o \
	him_code/hwmhrst.o \
	him_code/hwminit.o \
	him_code/hwmintr.o \
	him_code/hwmptcl.o \
	him_code/hwmscam.o \
	him_code/hwmse2.o \
	him_code/hwmtask.o \
	him_code/hwmutil.o \
	him_code/rsminit.o \
	him_code/rsmtask.o \
	him_code/rsmutil.o \
	him_code/xlminit.o \
	him_code/xlmtask.o \
	him_code/xlmutil.o \
	cio/instr.o \
	uerr/uni_err.o

OSMFILES = \
	adsl.c

CHIMFILES = \
	him_code/hwmdiag.c \
	him_code/hwmdlvr.c \
	him_code/hwmhrst.c \
	him_code/hwminit.c \
	him_code/hwmintr.c \
	him_code/hwmptcl.c \
	him_code/hwmscam.c \
	him_code/hwmse2.c \
	him_code/hwmtask.c \
	him_code/hwmutil.c \
	him_code/rsminit.c \
	him_code/rsmtask.c \
	him_code/rsmutil.c \
	him_code/xlminit.c \
	him_code/xlmtask.c \
	him_code/xlmutil.c

CIOFILES = \
	cio/instr.c

UERRFILES = \
	uerr/uni_err.c

SFILES =

CFILES = $(OSMFILES) $(CHIMFILES) $(CIOFILES) $(UERRFILES)

SRCFILES = $(CFILES) $(SFILES)

all:
	-@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(ADSL) $(MAKEARGS) "KBASE=$(KBASE)" \
		"LOCALDEF=$(LOCALDEF)" ;\
	else \
		if [ ! -r $(ADSL) ]; then \
			echo "ERROR: $(ADSL) is missing" 1>&2 ;\
			false ;\
		fi \
	fi

install:	all
		(cd adsl.cf ; $(IDINSTALL) -R$(CONF) -M adsl; \
		rm -f $(CONF)/pack.d/adsl/disk.cfg;	\
		cp disk.cfg $(CONF)/pack.d/adsl	)

$(ADSL):	$(FILES)
		$(LD) -r -o $(ADSL) $(FILES)

clean:
	-rm -f *.o him_code/*.o cio/*.o uerr/*.o $(LFILES) *.L $(ADSL)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e adsl

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(CHIMFILES:.c=.o):
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<
	-mv $(@F) $@

$(CIOFILES:.c=.o):
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<
	-mv $(@F) $@

$(UERRFILES:.c=.o):
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<
	-mv $(@F) $@

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
	adsl.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
