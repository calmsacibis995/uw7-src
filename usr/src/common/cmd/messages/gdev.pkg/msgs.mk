# gdev package message catalog makefile
#ident	"@(#)messages:common/cmd/messages/gdev.pkg/msgs.mk	1.2"

include $(CMDRULES)

PKG=gdev
CATALOG=$(PKG).pkg.str
MSGDIR=$(USRLIB)/locale/C/MSGFILES

all: msgs

install: all 
	cp msgs $(CATALOG)
	[ -d $(MSGDIR) ] || mkdir -p $(MSGDIR)
	$(INS) -f $(MSGDIR) -m 644 $(CATALOG)

clean :
	rm -f $(CATALOG)
