#ident	"@(#)kern-i386:util/ccnv/ccnv.mk	1.5"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	ccnv.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
INSDIR=/usr/lib/iconv/kmods
CCNV=ccnv.cf/Driver.o
DEFMOD=sb.o
OMODS=sjis/Driver.o
OMODOBJS=sjis.o

MODSTUB = ccnv.cf/Modstub.o
LFILE = $(LINTDIR)/ccnv.ln

LFILES = sb.ln \
	sjis.ln

CFILES = \
	sb.c \
	sjis.c

SRCFILES = $(CFILES)

all:	$(OMODS) $(CCNV) $(MODSTUB)

install: all
	(cd ccnv.cf; $(IDINSTALL) -R$(CONF) -M ccnv)
	[ -d $(ROOT)/$(MACH)/$(INSDIR)/437 ] || mkdir -p $(ROOT)/$(MACH)/$(INSDIR)/437
	$(INS) -f $(ROOT)/$(MACH)/$(INSDIR)/437 -m 444 -u bin -g bin ccnv.cf/Space.c
	$(INS) -f $(ROOT)/$(MACH)/$(INSDIR)/437 -m 444 -u bin -g bin ccnv.cf/Driver.o
	$(INS) -f $(ROOT)/$(MACH)/$(INSDIR)/437 -m 444 -u bin -g bin ccnv.cf/Modstub.o
	$(INS) -f $(ROOT)/$(MACH)/$(INSDIR)/437 -m 444 -u bin -g bin ccnv.cf/System
	$(INS) -f $(ROOT)/$(MACH)/$(INSDIR)/437 -m 444 -u bin -g bin ccnv.cf/Stubs.c
	$(INS) -f $(ROOT)/$(MACH)/$(INSDIR)/437 -m 444 -u bin -g bin ccnv.cf/Master
	[ -d $(ROOT)/$(MACH)/$(INSDIR)/850 ] || mkdir -p $(ROOT)/$(MACH)/$(INSDIR)/850
	$(INS) -f $(ROOT)/$(MACH)/$(INSDIR)/850 -m 444 -u bin -g bin 850/Space.c
	[ -d $(ROOT)/$(MACH)/$(INSDIR)/863 ] || mkdir -p $(ROOT)/$(MACH)/$(INSDIR)/863
	$(INS) -f $(ROOT)/$(MACH)/$(INSDIR)/863 -m 444 -u bin -g bin 863/Space.c
	[ -d $(ROOT)/$(MACH)/$(INSDIR)/865 ] || mkdir -p $(ROOT)/$(MACH)/$(INSDIR)/865
	$(INS) -f $(ROOT)/$(MACH)/$(INSDIR)/865 -m 444 -u bin -g bin 865/Space.c
	[ -d $(ROOT)/$(MACH)/$(INSDIR)/sjis ] || mkdir -p $(ROOT)/$(MACH)/$(INSDIR)/sjis
	$(INS) -f $(ROOT)/$(MACH)/$(INSDIR)/sjis -m 444 -u bin -g bin sjis/Driver.o

$(CCNV): $(DEFMOD)
	cp $(DEFMOD) $(CCNV)

sjis/Driver.o: sjis.o
	[ -d sjis ] || mkdir -p sjis
	cp sjis.o sjis/Driver.o

$(MODSTUB): ccnv_stub.o
	$(LD) -r -o $@ ccnv_stub.o 

clean:
	-rm -f $(DEFMOD) $(OMODOBJS) $(LFILES) *.L ccnv_stub.o 

clobber: clean
	-rm -f $(CCNV) $(OMODS)  $(MODSTUB)

lintit:	$(LFILE)

$(LINTDIR):
	-mkdir -p $@

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES);  \
	do \
		echo $$i; \
	done

headinstall: 

include $(UTSDEPEND)

include $(MAKEFILE).dep
