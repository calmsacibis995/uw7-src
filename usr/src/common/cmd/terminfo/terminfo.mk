#ident	"@(#)terminfo:common/cmd/terminfo/terminfo.mk	1.10.5.1"
#ident	"$Header$"

include $(CMDRULES)

#
#	terminfo makefile
#

TERMDIR =	$(USRSHARE)/lib/terminfo
TABDIR  =	$(USRSHARE)/lib/tabset
TERMCAPDIR =	$(USRSHARE)/lib


PARTS =		header *.ti trailer

all: ckdir terminfo.src osr5
	TERMINFO=$(TERMDIR) $(TIC) -v terminfo.src >errs 2>&1
	@touch install
	@echo
	@sh ./ckout
	@echo
	@echo
	@echo
	cp tabset/* $(TABDIR)
	$(CH)chmod 644 $(TABDIR)/*
	cp *.ti $(TERMDIR)/ti
	$(CH)chmod 644 $(TERMDIR)/ti/*.ti
	cp terminfo.src $(TERMDIR)
	$(CH)chmod 644 $(TERMDIR)/terminfo.src
	cp readme.bin $(TERMDIR)/README
	$(CH)chmod 444 $(TERMDIR)/README
	cp termcap $(TERMCAPDIR)
	$(CH)chmod 644 $(TERMCAPDIR)/termcap

osr5:
	cd OSR5; $(MAKE) TERMDIR=$(TERMDIR)

install: all
	cd OSR5; $(MAKE) TERMDIR=$(TERMDIR) install

terminfo.src: $(PARTS)
	@cat $(PARTS) > terminfo.src

clean:
	rm -f terminfo.src install errs nohup.out
	cd OSR5; $(MAKE) TERMDIR=$(TERMDIR) clean

clobber: clean
	cd OSR5; $(MAKE) TERMDIR=$(TERMDIR) clobber

ckdir:
	@echo
	@echo "The terminfo database will be built in $(TERMDIR)."
	@echo "Checking for the existence of $(TERMDIR):"
	@echo
	[ -d $(TERMDIR) ] || mkdir -p $(TERMDIR)
	$(CH)chown bin $(TERMDIR);
	$(CH)chgrp bin $(TERMDIR);
	$(CH)chmod 775 $(TERMDIR);
	@echo
	@echo
	@echo "The terminfo database will reference the tabset file in $(TABDIR)."
	@echo "Checking for the existence of $(TABDIR):"
	@echo
	[ -d $(TABDIR) ] || mkdir -p $(TABDIR)
	$(CH)chown bin $(TABDIR);
	$(CH)chgrp bin $(TABDIR);
	$(CH)chmod 775 $(TABDIR);
	@echo
	@echo
	@echo "The terminfo source files will be installed in $(TERMDIR)/ti."
	@echo "Checking for the existence of $(TERMDIR)/ti:"
	@echo
	[ -d $(TERMDIR)/ti ] || mkdir -p $(TERMDIR)/ti
	$(CH)chown root $(TERMDIR)/ti;
	$(CH)chgrp root $(TERMDIR)/ti;
	$(CH)chmod 775 $(TERMDIR)/ti;
	@echo
