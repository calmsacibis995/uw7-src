#		copyright	"%c%"
#ident	"@(#)Makefile	1.2"

#
# Makefile for the LP commands
#

include $(CMDRULES)


TOP	=	../..

include ../../common.mk


SCRIPTS	= \
		hpnp.mib	\
		trapd.conf	\
		bootptab	\
		tftp.cfg

all:


install:
	for cmd in $(SCRIPTS); \
	do \
		$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRLIBHP)/examples $$cmd; \
	done


clean:

clobber:

strip:
