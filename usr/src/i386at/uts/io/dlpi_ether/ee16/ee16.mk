#ident	"@(#)ee16.mk	28.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	ee16.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/dlpi_ether/ee16
LOCALDEF = -DEE16 -DESMP -DALLOW_SET_EADDR

EE16 = ee16.cf/Driver.o
LFILE = $(LINTDIR)/ee16.ln
PROBEFILE = ee16hrdw.c

FILES = \
	dlpi_ee16.o \
	ee16hrdw.o \
	ee16init.o \
	ee16asm.o

CFILES = \
	ee16hrdw.c \
	ee16init.c

SFILES = \
	ee16asm.s

LFILES = \
	ee16hrdw.ln \
	ee16init.ln \
	dlpi_ee16.ln

SRCFILES = $(CFILES) $(SFILES) ../dlpi_ether.c

.SUFFIXES: .ln

.c.ln:
	echo "\n$(DIR)/$*.c:" > $*.L
	-$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) \
		-DEE16 -c -u $*.c >> $*.L

all:
	-@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(EE16) $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
		if [ ! -r $(EE16) ]; then \
			echo "ERROR: $(EE16) is missing" 1>&2; \
			false; \
		fi \
	fi



install: all
	(cd ee16.cf; $(IDINSTALL) -R$(CONF) -M ee16)

$(EE16): $(FILES)
	$(LD) -r -o $(EE16) $(FILES)

clean:
	-rm -f *.o

clobber:	clean
	-if [ -f $(PROBEFILE) ]; then \
		rm -f $(EE16); \
	fi
	-$(IDINSTALL) -R$(CONF) -e -d ee16

$(LINTDIR):
	-mkdir -p $@

lintit: $(LFILE)
	-rm -f dlpi_ee16.c

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
	dlpi_ee16.h \
	ee16.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:

include $(UTSDEPEND)

include $(MAKEFILE).dep

#
# Special header dependencies
#
dlpi_ee16.o: ../dlpi_ether.c \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/buf_f.h \
	$(KBASE)/fs/ioccom.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/conssw.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/ddi_f.h \
	$(KBASE)/io/dlpi_ether/dlpi_ether.h \
	$(KBASE)/io/f_ddi.h \
	$(KBASE)/io/log/log.h \
	$(KBASE)/io/stream.h \
	$(KBASE)/io/strlog.h \
	$(KBASE)/io/strmdep.h \
	$(KBASE)/io/stropts.h \
	$(KBASE)/io/stropts_f.h \
	$(KBASE)/io/strstat.h \
	$(KBASE)/io/termio.h \
	$(KBASE)/io/termios.h \
	$(KBASE)/io/ttydev.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/net/dlpi.h \
	$(KBASE)/net/inet/byteorder.h \
	$(KBASE)/net/inet/byteorder_f.h \
	$(KBASE)/net/inet/if.h \
	$(KBASE)/net/inet/strioc.h \
	$(KBASE)/net/socket.h \
	$(KBASE)/net/sockio.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp_p.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/svc/clock.h \
	$(KBASE)/svc/clock_p.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/intr.h \
	$(KBASE)/svc/reg.h \
	$(KBASE)/svc/secsys.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/svc/trap.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/dl.h \
	$(KBASE)/util/engine.h \
	$(KBASE)/util/ipl.h \
	$(KBASE)/util/kdb/kdebugger.h \
	$(KBASE)/util/ksinline.h \
	$(KBASE)/util/ksynch.h \
	$(KBASE)/util/ksynch_p.h \
	$(KBASE)/util/list.h \
	$(KBASE)/util/listasm.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/param_p.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/sysmacros_f.h \
	$(KBASE)/util/types.h \
	$(FRC)
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c ../dlpi_ether.c && \
		mv dlpi_ether.o dlpi_ee16.o

