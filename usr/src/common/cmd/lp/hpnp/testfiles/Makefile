#		copyright	"%c%"
#ident	"@(#)Makefile	1.2"

#
# Makefile for the LP commands
#

include $(CMDRULES)


TOP	=	../..

include ../../common.mk


SCRIPTS	= \
		ps		\
		text

all:


install:
	for cmd in $(SCRIPTS); \
	do \
		$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRLIBHP)/testfiles $$cmd; \
	done


clean:

clobber:

strip:
