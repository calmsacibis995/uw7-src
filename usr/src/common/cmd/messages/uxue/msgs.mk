#	copyright	"%c%"

#ident	"@(#)messages:common/cmd/messages/uxue/msgs.mk	1.1.3.3"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs ue.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 ue.str

lintit : 

clean :
	rm -f ue.str

clobber : clean

