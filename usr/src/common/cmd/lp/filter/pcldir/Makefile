#ident	"@(#)Makefile	1.2"
#ident	"$Header$"

#
# Makefile for lp/filter/pcl/pcl.fd
#


include $(CMDRULES)

TOP	=	../..

include ../../common.mk


FDTMP	=	$(ETCLP)/fd


SRCS	= \
		pcl.fd


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

