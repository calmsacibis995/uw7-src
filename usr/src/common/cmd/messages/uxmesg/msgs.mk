#	copyright	"%c%"

#ident	"@(#)messages:common/cmd/messages/uxmesg/msgs.mk	1.1.3.3"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs mesg.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 mesg.str

lintit : 

clean :
	rm -f mesg.str

clobber : clean

