#ident	"@(#)kern-i386at:io/layer/mpio/mpio.mk	1.2.6.2"

include $(UTSRULES)

MAKEFILE=	mpio.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/target/mpio

MPIO = mpio.cf/Driver.o
LFILE = $(LINTDIR)/mpio.ln

FILES = mpio.o \
	mpio_ioctl.o \
	mpio_core.o \
	mpio_msg.o \
	mpio_cmac.o \
	mpio_qm.o \
	mpio_os.o

CFILES = mpio.c \
	mpio_ioctl.c \
	mpio_msg.c \
	mpio_core.c \
	mpio_cmac.c \
	mpio_qm.c \
	mpio_os.c

LFILES = mpio.ln \
	mpio_ioctl.ln \
	mpio_msg.ln \
	mpio_core.ln \
	mpio_cmac.ln \
	mpio_qm.ln \
	mpio_os.ln

SRCFILES = $(CFILES)

all:	$(MPIO)

install:	all
		(cd mpio.cf ; $(IDINSTALL) -R$(CONF) -M mpio)

$(MPIO):	$(FILES)
		$(LD) -r -o $(MPIO) $(FILES)
		# $(FUR) -W -o mpio.order $(MPIO)

clean:
	-rm -f *.o $(LFILES) *.L $(MPIO)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e mpio

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

sysHeaders = \
	mpio.h \
	mpio_ioctl.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
