#	copyright	"%c%"

#ident	"@(#)stdcmds:suscfg/suscfg.mk	1.1"

include $(CMDRULES)

U95DIR=$(ROOT)/$(MACH)/u95
U95BIN=$(U95DIR)/bin

OWN = root
GRP = sys

MAKEFILE = suscfg.mk

MAINS = suscfg

SOURCES = suscfg.sh

all:	$(MAINS)

suscfg: suscfg.sh

clobber: 
	rm -f $(MAINS)

install: all uxsuscfg.str
	if [ ! -d $(U95DIR) ] ; \
	then \
		mkdir $(U95DIR) ; \
	fi
	if [ ! -d $(U95BIN) ] ; \
	then \
		mkdir $(U95BIN) ; \
	fi
	$(INS) -f $(U95BIN) -m 0500 -u $(OWN) -g $(GRP) suscfg

	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES uxsuscfg.str

remove:
	cd $(U95BIN);  rm -f $(MAINS)
	rm -f $(USRLIB)/locale/C/MSGFILES/uxsuscfg.str

