#	copyright	"%c%"

#ident	"@(#)messages:common/cmd/messages/uxpkgtools/msgs.mk	1.2"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs pkgtools.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 pkgtools.str

lintit : 

clean :
	rm -f pkgtools.str

clobber : clean

