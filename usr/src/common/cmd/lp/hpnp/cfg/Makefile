#ident	"@(#)Makefile	1.2"
#		copyright	"%c%"

#
# Makefile for the LP commands
#

include $(CMDRULES)


TOP	=	../..

include ../../common.mk


SCRIPTS	= \
		bootp.awk	\
		bootptabadd	\
		getaccesslist	\
		getbootpha	\
		getdefgateway	\
		gethatag	\
		getidleto	\
		getipaddr	\
		getname 	\
		getspoolname	\
		getsubnetmask	\
		getsyslog	\
		gettrapdest	\
		isipinhost	\
		onlyoneaddr	\
		option1		\
		option2		\
		option3		\
		option4		\
		option5		\
		option6		\
		option7		\
		option8		\
		putpnameinhost	\
		rmbootpentry	\
		siginet

all:


install:
	for cmd in $(SCRIPTS); \
	do \
		$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRLIBHP)/cfg $$cmd; \
	done


clean:

clobber:

strip:
