#ident	"@(#)Makefile	1.2"
#ident	"$Header$"

#
# Makefile for lp/filter/lpd/catv.fd
# and lp/filter/lpd/prtitle.fd
#


include $(CMDRULES)

TOP	=	../..

include ../../common.mk


FDTMP	=	$(ETCLP)/fd


SRCS	=	catv.fd \
		prtitle.fd


all:		$(SRCS)

install:	ckdir
	cp $(SRCS) $(FDTMP)

ckdir:
	@if [ ! -d $(FDTMP) ]; \
	then \
		mkdir $(FDTMP); \
		$(CH)chgrp $(GROUP) $(FDTMP); \
		$(CH)chown $(OWNER) $(FDTMP); \
		chmod $(DMODES) $(FDTMP); \
	fi

lintit:

clean:

clobber:	clean

