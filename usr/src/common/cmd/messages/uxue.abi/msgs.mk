#	copyright	"%c%"

#ident	"@(#)messages:common/cmd/messages/uxue.abi/msgs.mk	1.1.3.3"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs ue.abi.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 ue.abi.str

lintit : 

clean :
	rm -f ue.abi.str

clobber : clean

