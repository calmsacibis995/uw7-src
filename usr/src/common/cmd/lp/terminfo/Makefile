#ident	"@(#)Makefile	1.2"
#ident	"$Header$"

#
# Makefile for the Terminfo entries delivered with LP
#


include $(CMDRULES)

TOP	=	..

include ../common.mk


TERMINFO=	$(USRSHARELIB)/terminfo
TITMP	=	$(TERMINFO)/ti


COMPILE	=	tic -v


SRCS	= \
		PS.ti

all:
		
install:	ckdir \
		terminfo.src
	TERMINFO=$(TERMINFO) $(COMPILE) terminfo.src
	cp $(SRCS) $(TITMP)

clean:
	$(RM) terminfo.src

clobber:	clean

strip:

terminfo.src:	$(SRCS)
	@cat $(SRCS) >terminfo.src

ckdir:
	@if [ ! -d $(TERMINFO) ]; \
	then \
		mkdir $(TERMINFO); \
		$(CH)chgrp bin $(TERMINFO); \
		$(CH)chown bin $(TERMINFO); \
		chmod 775 $(TERMINFO); \
	fi
	@if [ ! -d $(TITMP) ]; \
	then \
		mkdir $(TITMP); \
		$(CH)chgrp root $(TITMP); \
		$(CH)chown root $(TITMP); \
		chmod $(DMODES) $(TITMP); \
	fi

lintit:

lintlib:
