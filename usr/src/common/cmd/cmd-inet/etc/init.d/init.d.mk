#ident	"@(#)init.d.mk	1.12"
#
# Copyrighted as an unpublished work.
# (c) Copyright 1987-1997 Computer Associates International, Inc.
# All rights reserved.
#
#      SCCS IDENTIFICATION

include $(CMDRULES)

OWN=		root
GRP=		sys
INITD=		$(ETC)/init.d
ETCNETB=	$(ETC)/netbios
STARTNETB=	$(ETC)/rc2.d/S74netbios
STOPNETB1=	$(ETC)/rc1.d/K68netbios
STOPNETB0=	$(ETC)/rc0.d/K68netbios
STARTINET=	$(ETC)/rc2.d/S69inet
STOPINET1=	$(ETC)/rc1.d/K69inet
STOPINET0=	$(ETC)/rc0.d/K69inet
INITD=          $(ETC)/init.d

DIRS=		$(ETC)/rc2.d $(ETC)/rc1.d $(ETC)/rc0.d $(INITD)

all: netbios

netbios:	netbiosrc_msg.sh netbios.sh
		echo ":" > $@
		cat netbiosrc_msg.sh >> $@
		cat netbios.sh >> $@

netbiosrc_msg.sh:		NLS/english/netbiosrc.gen
	mkcatdefs -s netbiosrc NLS/english/netbiosrc.gen > /dev/null

$(DIRS):
	[ -d $@ ] || mkdir -p $@

install: $(DIRS) all
	$(INS) -f $(INITD) -m 0444 -u $(OWN) -g $(GRP) inetinit
	rm -f $(STARTINET) $(STOPINET1) $(STOPINET0)
	-ln $(INITD)/inetinit $(STARTINET)
	-ln $(INITD)/inetinit $(STOPINET1)
	-ln $(INITD)/inetinit $(STOPINET0)
	$(INS) -f $(INITD) -m 0444 -u $(OWN) -g $(GRP) netbios
	rm -f $(ETCNETB) $(STARTNETB) $(STOPNETB1) $(STOPNETB0)
	-ln $(INITD)/netbios $(STARTNETB)
	-ln $(INITD)/netbios $(STOPNETB1)
	-ln $(INITD)/netbios $(STOPNETB0)
	-ln $(INITD)/netbios $(ETCNETB)

	@-if [ -x "$(DOCATS)" ]; \
	then \
		$(DOCATS) -d NLS $@ ; \
	fi

clean:
	-rm -f netbios netbiosrc_msg.sh
	-rm -f NLS/*/*cat* NLS/*/temp

clobber: clean

depend:
