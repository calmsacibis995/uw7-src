# fstyp message catalog makefile
#ident	"@(#)messages:common/cmd/messages/fstyp/msgs.mk	1.1"

include $(CMDRULES)

CATALOG=fstyp.str
MSGDIR=$(USRLIB)/locale/C/MSGFILES

all: msgs

install: all 
	cp msgs $(CATALOG)
	[ -d $(MSGDIR) ] || mkdir -p $(MSGDIR)
	$(INS) -f $(MSGDIR) -m 644 $(CATALOG)

clean :
	rm -f $(CATALOG)

clobber:
	clean
