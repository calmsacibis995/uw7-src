# eth package message catalog makefile
#ident	"@(#)messages:common/cmd/messages/eth.pkg/msgs.mk	1.2"

include $(CMDRULES)

PKG=eth
CATALOG=$(PKG).pkg.str
MSGDIR=$(USRLIB)/locale/C/MSGFILES

all: msgs

install: all 
	cp msgs $(CATALOG)
	[ -d $(MSGDIR) ] || mkdir -p $(MSGDIR)
	$(INS) -f $(MSGDIR) -m 644 $(CATALOG)

clean :
	rm -f $(CATALOG)
