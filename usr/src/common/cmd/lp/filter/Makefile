#	copyright	"%c%"

#ident	"@(#)Makefile	1.2"
#ident "$Header$"
#
# Makefile for lp/filter
#



include $(CMDRULES)

TOP	=	..

include ../common.mk


MISC	= \
		slow.filter \
		pcl \
		I18N_pp \
		I18N_print \
		I18N_wrap


all:		$(MISC)
	cd postscript; $(MAKE) $(MAKEARGS) all
	cd pcldir; $(MAKE) $(MAKEARGS) all
	cd i18n; $(MAKE) $(MAKEARGS) all
	cd wrap; $(MAKE) $(MAKEARGS) all
	cd lpd; $(MAKE) $(MAKEARGS) all

install:	all
	for misc in $(MISC); \
	do \
		$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(LPBINDIR) $$misc; \
	done
	cd postscript; $(MAKE) $(MAKEARGS) install
	cd pcldir; $(MAKE) $(MAKEARGS) install
	cd i18n; $(MAKE) $(MAKEARGS) install
	cd wrap; $(MAKE) $(MAKEARGS) install
	cd lpd; $(MAKE) $(MAKEARGS) install

clean clobber:
	cd postscript; $(MAKE) $(MAKEARGS) $@
	cd pcldir; $(MAKE) $(MAKEARGS) $@
	cd i18n; $(MAKE) $(MAKEARGS) $@
	cd wrap; $(MAKE) $(MAKEARGS) $@
	cd lpd; $(MAKE) $(MAKEARGS) $@

pcl:
	cp pcldir/pcl.filter pcl

I18N_pp:
	cp i18n/I18N_pp.filter I18N_pp

I18N_print:
	cp i18n/I18N_print.filter I18N_print

I18N_wrap:
	cp wrap/I18N_wrap.filter I18N_wrap

strip:

lintit:

lintlib:
