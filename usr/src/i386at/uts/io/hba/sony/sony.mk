#ident	"@(#)kern-i386at:io/hba/sony/sony.mk	1.2"
#ident	"$Header$"

include $(UTSRULES)

.SUFFIXES: .ln
.c.ln:
	echo "\n$(DIR)/$*.c:" > $*.L
	-$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) \
		-c -u $*.c >> $*.L

MAKEFILE=	sony.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/sony
LOCALDEF =  -DUWONEDOTONE

SONY = sony.cf/Driver.o
CD535 = cd535.cf/Driver.o
CD31A = cd31a.cf/Driver.o
LFILE = $(LINTDIR)/sony.ln

FILES = hba.o sony31a.o sony535.o scsifake.o

CFILES = hba.c sony31a.c sony535.c scsifake.c
LFILES = hba.ln sony31a.ln sony535.ln scsifake.ln

SRCFILES = $(CFILES)

all:	$(SONY) $(CD535) $(CD31A)

install:	all
		(cd sony.cf ; $(IDINSTALL) -R$(CONF) -M sony; \
		rm -f $(CONF)/pack.d/sony/disk.cfg;	\
		cp disk.cfg $(CONF)/pack.d/sony	);	\
		(cd cd535.cf ; $(IDINSTALL) -R$(CONF) -M cd535);	\
		(cd cd31a.cf ; $(IDINSTALL) -R$(CONF) -M cd31a)

$(SONY):	$(FILES)
		$(LD) -r -o $(SONY) $(FILES)

$(CD535):	cd535.o
		$(LD) -r -o $(CD535) cd535.o

$(CD31A):	cd31a.o
		$(LD) -r -o $(CD31A) cd31a.o

clean:
	-rm -f *.o $(LFILES) *.L $(SONY)
	-rm -f *.o $(LFILES) *.L $(CD535)
	-rm -f *.o $(LFILES) *.L $(CD31A)
	

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e sony
	$(IDINSTALL) -R$(CONF) -d -e cd535
	$(IDINSTALL) -R$(CONF) -d -e cd31a

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
	sony.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include	$(UTSDEPEND)

include	$(MAKEFILE).dep
