#ident	"@(#)kern-i386at:io/hba/blc/blc.mk	1.2.2.4"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	blc.mk
KBASE = ../../..
LOCALDEF = -D_KERNEL -DMMDRV -DMM_ONLY
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/blc

BLC = blc.cf/Driver.o
BINARIES = $(BLC)
LFILE = $(LINTDIR)/blc.ln

UCBCFILES = \
	ucbmgr/budi.c \
	ucbmgr/ucbdat.c \
	ucbmgr/ucbmgr.c

UCBHFILES = \
	ucbmgr/multimas.h
UCBINCLIST = -Iucbmgr

UCBDEFLIST = -DUNIX -D_KERNEL -DMMDRV -DMM_ONLY

FILES = \
	budidrv.o
CFILES = \
	budidrv.c

SFILES =

LFILES = blc.ln

PROBEFILE = budidrv.c

SRCFILES = $(CFILES) $(UCBCFILES) $(SFILES)

all:
	echo "BLC is  " $(BLC);
	-@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(BLC) $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
		if [ ! -r $(BLC) ]; then \
			echo "ERROR: $(BLC) is missing" 1>&2 ;\
			false ;\
		fi \
	fi

install:	all
		(cd blc.cf ; $(IDINSTALL) -R$(CONF) -M blc; \
		rm -f $(CONF)/pack.d/blc/disk.cfg;	\
		cp disk.cfg $(CONF)/pack.d/blc	)

$(BLC): $(FILES)	
		$(LD) -r -o $(BLC) $(FILES) 
#		$(FUR) -W -o blc.order $(BLC)

clean:
	-rm -f *.o $(LFILES) *.L
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e blc 

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(UCBCFILES:.c=.o):
	$(CC) $(CFLAGS) $(INCLIST) $(UCBINCLIST) $(DEFLIST) $(UCBDEFLIST) -c $<
	echo $@
	-mv `expr $@ : 'ucbmgr/\(.*\)'` $@

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
	blc.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
