#ident	"@(#)msgs.mk	1.2"
#ident  "$Header$"

include $(CMDRULES)

PKG=nis
CATALOG=$(PKG).pkg.str
MSGDIR=$(USRLIB)/locale/C/MSGFILES

all	: msgs

install: all 
	cp msgs $(CATALOG)
	[ -d $(MSGDIR) ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 $(CATALOG)

lintit : 

clean :
	rm -f $(CATALOG)

clobber : clean

