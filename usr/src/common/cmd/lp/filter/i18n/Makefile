#ident	"@(#)Makefile	1.2"
#ident	"$Header$"

#
# Makefile for lp/filter/i18n/i18n_pp.fd
# and
# for lp/filter/i18n/i18n_print.fd
#


include $(CMDRULES)

TOP	=	../..

include ../../common.mk


FDTMP	=	$(ETCLP)/fd


LCLDIR	=	$(USRLIBLP)/locale


LCLDIR_C	=	$(LCLDIR)/C


ALLDIRS	=	$(FDTMP) \
		$(LCLDIR) \
		$(LCLDIR_C)


SRCS	= \
		i18n_pp.fd \
		i18n_print.fd \
		pcl_locale.fd \
		PS_locale.fd


LCLFILES	= \
		LCL_CHARSET \
		LCL_PRINT \
		LCL_PS


all:

install:	ckdir
	cp $(SRCS) $(FDTMP)
	cp $(LCLFILES) $(LCLDIR_C)

clean:

clobber:	clean

strip:

ckdir:
	for CKDIR in $(ALLDIRS); \
	do \
	if [ ! -d $$CKDIR ]; \
	then \
		mkdir $$CKDIR; \
		$(CH)chgrp $(GROUP) $$CKDIR; \
		$(CH)chown $(OWNER) $$CKDIR; \
		chmod $(DMODES) $$CKDIR; \
	fi \
	done

lintit:

