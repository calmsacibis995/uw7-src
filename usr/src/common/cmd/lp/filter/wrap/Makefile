#ident	"@(#)Makefile	1.2"
#ident	"$Header$"

#
# Makefile for lp/filter/wrap/wrap.fd
# and lp/filter/wrap/wrap
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


LCLFILES	= \
		LCL_WRAP


CFILES	=	wrap.c


SRCS	=	wrap.fd


all:		wrap

install:	ckdir wrap
	cp $(SRCS) $(FDTMP)
	cp $(LCLFILES) $(LCLDIR_C)
	@if [ ! -d "$(LPBINDIR)" ]; then \
	    mkdir $(LPBINDIR); \
	    $(CH) chmod 775 $(LPBINDIR); \
	    $(CH) chgrp $(GROUP) $(LPBINDIR); \
	    $(CH) chown $(OWNER) $(LPBINDIR); \
	fi
	$(INS) -m 775 -u $(OWNER) -g $(GROUP) -f $(LPBINDIR) wrap


wrap : 		wrap.o
	$(CC) -o wrap  wrap.o $(SHLIBS)

clean :
	rm -f wrap.o

clobber : clean
	rm -f wrap

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
