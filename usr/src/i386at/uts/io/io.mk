#ident	"@(#)kern-i386at:io/io.mk	1.113.12.3"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	io.mk
KBASE = ..
LINTDIR = $(KBASE)/lintdir
DIR = io

LFILE = $(LINTDIR)/io.ln

IO = io.cf/Driver.o
DCOMPAT = dcompat.cf/Driver.o

FILES = \
	autoconf.o \
	compat_subr.o \
	conssw.o \
	ddi.o \
	ddi_f.o \
	ddi_misc.o \
	ddi386at.o \
	ddislp.o \
	dkibind.o \
	metdisk.o \
	physio.o \
	strcalls.o \
	stream.o \
	streamio.o \
	streamio_p.o \
	strsubr.o \
	tcspeed.o

CFILES = \
	autoconf.c \
	compat_subr.c \
	conssw.c \
	dcompat.c \
	ddi.c \
	ddi_f.c \
	ddi386at.c \
	ddislp.c \
	dkibind.c \
	metdisk.c \
	physio.c \
	strcalls.c \
	stream.c \
	streamio.c \
	streamio_p.c \
	strsubr.c \
	tcspeed.c

SFILES = \
	ddi_misc.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	autoconf.ln \
	compat_subr.ln \
	conssw.ln \
	dcompat.ln \
	ddi.ln \
	ddi_f.ln \
	ddi386at.ln \
	ddislp.ln \
	dkibind.ln \
	metdisk.ln \
	physio.ln \
	strcalls.ln \
	stream.ln \
	streamio.ln \
	streamio_p.ln \
	strsubr.ln \
	tcspeed.ln

SUBDIRS = nd autoconf bios osm kd cram ws kdvm sad ldterm log uni pipemod \
	gentty connld dma nullzero mem rtc prf lockstat pt dlpi_ether \
	lp mfpd char cmux ansi gvid fd ttcompat asy iaf xque tp mse hba target layer \
	devadp sysmsg event consem sp random kbd alp intmap ramd postwait \
	async odi crom cpqw mirrorcon udev cpyrt \
	xpoll gsd fnt rlogin pccard mtrr cpumtr cgmtr i2o \
	vxvm vxportal


LINTDIRS = nd autoconf bios osm kd cram ws kdvm sad ldterm log uni pipemod \
	gentty connld dma nullzero mem rtc prf lockstat pt \
	lp char cmux ansi gvid fd ttcompat asy iaf xque tp mse hba target layer \
	devadp sysmsg event consem sp random kbd alp intmap postwait async \
	crom cpqw mirrorcon udev xpoll gsd fnt cpyrt \
	rlogin pccard vxportal

LINT_NOT_WORKING = dlpi_ether ramd

all:	local FRC
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
		 $(MAKE) -f $$d.mk all $(MAKEARGS)); \
		fi; \
	 done

local:	$(IO) $(DCOMPAT)

$(IO): $(FILES)
	$(LD) -r -o $(IO) $(FILES)
	$(FUR) -W -o io.order $(IO)

$(DCOMPAT): dcompat.o
	$(LD) -r -o $(DCOMPAT) dcompat.o

ddi_misc.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

install: localinstall FRC
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk install"; \
		 $(MAKE) -f $$d.mk install $(MAKEARGS)); \
		fi; \
	 done

localinstall: local FRC
	cd io.cf; $(IDINSTALL) -R$(CONF) -M io
	cd dcompat.cf; $(IDINSTALL) -R$(CONF) -M dcompat

clean:	localclean
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clean"; \
		 $(MAKE) -f $$d.mk clean $(MAKEARGS)); \
		fi; \
	 done

localclean:
	-rm -f *.o $(LFILES) *.L $(DCOMPAT) $(IO)

localclobber:	localclean
	-$(IDINSTALL) -R$(CONF) -d -e io
	-$(IDINSTALL) -R$(CONF) -d -e dcompat

clobber:	localclobber
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
		 $(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
		fi; \
	 done

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE) FRC
	@for d in $(LINTDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk lintit"; \
		 $(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
		fi; \
	 done

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
	    (cd $$d; \
		$(MAKE) -f $$d.mk fnames $(MAKEARGS) | \
		$(SED) -e "s;^;$$d/;"); \
		fi; \
	done
	@for f in $(SRCFILES); do \
		echo $$f; \
	done

headinstall: localhead FRC
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
		 $(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
		fi; \
	 done

sysHeaders = \
	ascii.h \
	conf.h \
	conssw.h \
	ddi.h \
	ddicheck.h \
	dma.h \
	elog.h \
	f_ddi.h \
	i8237A.h \
	iobuf.h \
	ioctl.h \
	jioctl.h \
	metdisk.h \
	metdisk_p.h \
	mkdev.h \
	mouse.h \
	open.h \
	ddi_f.h \
	ddi_i386at.h \
	poll.h \
	scgth.h \
	stermio.h \
	stream.h \
	strlog.h \
	strmdep.h \
	stropts.h \
	stropts_f.h \
	strstat.h \
	strsubr.h \
	strsubr_f.h \
	strtty.h \
	syslog.h \
	termio.h \
	termios.h \
	termiox.h \
	ttold.h \
	tty.h \
	ttychars.h \
	ttydev.h \
	uio.h \
	vtoc.h

localhead: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:

include $(UTSDEPEND)

depend::
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		 touch $$d.mk.dep;\
		 $(MAKE) -f $$d.mk depend $(MAKEARGS));\
		fi; \
	done

include $(MAKEFILE).dep

