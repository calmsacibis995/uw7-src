#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved

#	This Module contains Proprietary Information of Microsoft 
#	Corporation and should be treated as Confidential.

#ident	"@(#)acp:i386/cmd/acp/acp.mk	1.2.3.1"

include	$(CMDRULES)

LDFLAGS=-lgen
COMMANDS = fsck mount lang
FRC =
EVENT = $(USRLIB)/event

all:
	for cmd in $(COMMANDS) ; \
	do \
		(cd $$cmd; $(MAKE) -f $$cmd.mk $(MAKEARGS) all); \
	done

install: $(EVENT)
	for cmd in $(COMMANDS) ; \
	do \
		(cd $$cmd; $(MAKE) -f $$cmd.mk $(MAKEARGS) install); \
	done
	$(INS) -f $(EVENT) -m 644 -u bin -g bin devices
	$(INS) -f $(EVENT) -m 644 -u bin -g bin ttys
	$(INS) -f $(ETC) -m 644 -u bin -g bin socket.conf

$(EVENT):
	-mkdir -p $@
	$(CH)chmod 755 $@
	$(CH)chgrp sys $@
	$(CH)chown root $@

clean:
	for cmd in $(COMMANDS) ; \
	do \
		(cd $$cmd; $(MAKE) -f $$cmd.mk clean); \
	done

clobber:	clean
