#	copyright	"%c%"

#ident	"@(#)Makefile	1.2"
#ident "$Header$"

#
# Makefile for lp/crontab
#

include $(CMDRULES)

CRONTAB	=	$(VAR)/spool/cron/crontabs

OWNER	=	root
GROUP	=	root
MODE	=	444

SRC	=	lp

all:

install:
	$(INS) -m $(MODE) -u $(OWNER) -g $(GROUP) -f $(CRONTAB) $(SRC)

clobber:

clean:

strip:

lintit:

lintlib:
