#	copyright	"%c%"

#ident	"@(#)messages:common/cmd/messages/uxcore/msgs.mk	1.1.3.3"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs core.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 core.str

lintit : 

clean :
	rm -f core.str

clobber : clean

