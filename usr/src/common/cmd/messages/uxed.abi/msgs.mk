#	copyright	"%c%"

#ident	"@(#)messages:common/cmd/messages/uxed.abi/msgs.mk	1.1.3.3"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs ed.abi.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 ed.abi.str

lintit : 

clean :
	rm -f ed.abi.str

clobber : clean

