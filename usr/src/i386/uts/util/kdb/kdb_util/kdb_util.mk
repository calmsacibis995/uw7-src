#ident	"@(#)kern-i386:util/kdb/kdb_util/kdb_util.mk	1.16.3.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	kdb_util.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = util/kdb/kdb_util

KDBUTIL = kdb_util.cf/Driver.o
KDBUTIL_CCNUMA = ../kdb_util.cf/Driver_ccnuma.o
LFILE = $(LINTDIR)/kdb_util.ln

CCNUMA_MODULES = \
		$(KDBUTIL_CCNUMA)

FILES = \
	bits.o \
	db_as.o \
	extn.o \
	kdb.o \
	kdb_p.o \
	opset.o \
	stacktrace.o \
	tbls.o \
	utls.o

CFILES = \
	bits.c \
	db_as.c \
	extn.c \
	kdb.c \
	kdb_p.c \
	opset.c \
	stacktrace.c \
	tbls.c \
	utls.c

SRCFILES = $(CFILES)

LFILES = \
	bits.ln \
	db_as.ln \
	extn.ln \
	kdb.ln \
	kdb_p.ln \
	opset.ln \
	stacktrace.ln \
	tbls.ln \
	utls.ln


all:	$(KDBUTIL) kdb_util.cf/Modstub.o ccnuma

ccnuma: 
	if [ "$$DUALBUILD" = 1 ]; then \
		if [ ! -d ccnuma.d ]; then \
			mkdir ccnuma.d; \
			cd ccnuma.d; \
			for file in "../*.[csh] ../*.mk*"; do \
				ln -s $$file . ; \
			done; \
		else \
			cd ccnuma.d; \
		fi; \
		$(MAKE) -f kdb_util.mk $(CCNUMA_MAKEARGS) $(CCNUMA_MODULES); \
	fi

install: all
	(cd kdb_util.cf; $(IDINSTALL) -M -R$(CONF) kdb_util)

$(KDBUTIL): $(FILES)
	$(LD) -r -o $(KDBUTIL) $(FILES)

$(KDBUTIL_CCNUMA): $(FILES)
	$(LD) -r -o $(KDBUTIL_CCNUMA) $(FILES)

kdb_util.cf/Modstub.o:	kdb_util.cf/Stubs.c kdb_stub.o
	$(CC) -c $(CFLAGS) $(INCLIST) $(DEFLIST) -DMODSTUB kdb_util.cf/Stubs.c
	$(LD) -r -o kdb_util.cf/Modstub.o Stubs.o kdb_stub.o
	-rm -f Stubs.o

clean:
	-rm -f *.o $(LFILES) *.L $(CCNUMA_MODULES) \
		$(KDBUTIL) kdb_util.cf/Modstub.o
	-rm -rf ccnuma.d

clobber: clean
	$(IDINSTALL) -R$(CONF) -d kdb_util

lintit:	$(LFILE)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

$(LINTDIR):
	-mkdir -p $@

$(LFILE): $(LINTDIR) $(LFILES)

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

headinstall:



include $(UTSDEPEND)

include $(MAKEFILE).dep
