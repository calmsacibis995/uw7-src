#	copyright	"%c%"

#ident	"@(#)sulogin.mk	1.4"

include $(CMDRULES)

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved
#	This Module contains Proprietary Information of Microsoft
#	Corporation and should be treated as Confidential.

OWN = root
GRP = sys

LDLIBS = -lcmd -lcrypt -liaf -lia -lgen

MSGS = sulogin.str

all: sulogin

sulogin: sulogin.o 
	$(CC) -o $@ $@.o  $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

sulogin.o: sulogin.c \
	$(INC)/sys/types.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/stdio.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/string.h \
	$(INC)/ia.h \
	$(INC)/utmpx.h \
	$(INC)/unistd.h \
	$(INC)/priv.h \
	$(INC)/mac.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/secsys.h \
	$(INC)/limits.h 

clean:
	rm -f sulogin.o
	
clobber: clean
	rm -f sulogin

lintit:
	$(LINT) $(LINTFLAGS) sulogin.c

install: sulogin $(MSGS)
	-rm -f $(ETC)/sulogin
	 $(INS) -f $(SBIN) -m 0550 -u $(OWN) -g $(GRP) sulogin
	-$(SYMLINK) /sbin/sulogin $(ETC)/sulogin
	-[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 sulogin.str

# optional targets

save:
	cd $(SBIN); set -x; for m in sulogin; do cp $$m OLD$$m; done

restore:
	cd $(SBIN); set -x; for m in sulogin; do; cp OLD$$m $$m; done

remove:
	cd $(SBIN); rm -f sulogin

partslist:
	@echo sulogin.mk $(LOCALINCS) sulogin.c | tr ' ' '\012' | sort

product:
	@echo sulogin | tr ' ' '\012' | \
	sed -e 's;^;$(SBIN)/;' -e 's;//*;/;g'

productdir:
	@echo $(SBIN)

srcaudit: # will not report missing nor present object or product files.
	@fileaudit sulogin.mk $(LOCALINCS) sulogin.c -o sulogin.o sulogin
