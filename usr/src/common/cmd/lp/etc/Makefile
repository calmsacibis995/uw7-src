#	copyright	"%c%"

#ident	"@(#)Makefile	1.2"
#ident "$Header$"
#
# Makefile for lp/etc
#


include $(CMDRULES)

TOP	=	..

include ../common.mk


INIT_D	=	$(ETC)/init.d
RC0_D	=	$(ETC)/rc0.d
RC2_D	=	$(ETC)/rc2.d

OWNER	=	root
GROUP	=	sys
RMODES	=	0444


all:

install:
	$(INS) -m $(RMODES) -u $(OWNER) -g $(GROUP) -f $(INIT_D) lp
	-$(LN) $(INIT_D)/lp $(RC0_D)/K20lp
	-$(LN) $(INIT_D)/lp $(RC2_D)/S80lp

clean:

clobber:

strip:

lintit:

lintlib:
