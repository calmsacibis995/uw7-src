#	copyright	"%c%"

#ident	"@(#)messages:common/cmd/messages/uxcs/msgs.mk	1.1"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs uxcs.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 uxcs.str

lintit : 

clean :
	rm -f uxcs.str

clobber : clean

