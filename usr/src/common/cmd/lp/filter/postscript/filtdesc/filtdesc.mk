#ident	"@(#)filtdesc.mk	1.2"
#ident	"$Header$"

#
# Makefile for lp/filter/postscript/filtdesc
#


include $(CMDRULES)

TOP	=	../../..

include ../../../common.mk


FDTMP	=	$(ETCLP)/fd


SRCS	= \
		download.fd \
		dpost.fd \
		postdaisy.fd \
		postdmd.fd \
		postio.fd \
		postior.fd \
		postio_b.fd \
		postio_r.fd \
		postio_br.fd \
		postmd.fd \
		postplot.fd \
		postprint.fd \
		postreverse.fd \
		posttek.fd


all:

install:	ckdir
	cp $(SRCS) $(FDTMP)

clean:

clobber:	clean

strip:

ckdir:
	@if [ ! -d $(FDTMP) ]; \
	then \
		mkdir $(FDTMP); \
		$(CH)chgrp $(GROUP) $(FDTMP); \
		$(CH)chown $(OWNER) $(FDTMP); \
		chmod $(DMODES) $(FDTMP); \
	fi

lintit:

