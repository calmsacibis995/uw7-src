#	copyright	"%c%"

#ident	"@(#)messages:common/cmd/messages/uxrc/msgs.mk	1.1"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs uxrc.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 uxrc.str

lintit : 

clean :
	rm -f uxrc.str

clobber : clean

