#	copyright	"%c%"

#ident	"@(#)messages:common/cmd/messages/uxcore.abi/msgs.mk	1.1.3.3"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs core.abi.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 core.abi.str

lintit : 

clean :
	rm -f core.abi.str

clobber : clean

