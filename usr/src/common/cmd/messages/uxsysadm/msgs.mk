#	copyright	"%c%"

#ident	"@(#)messages:common/cmd/messages/uxsysadm/msgs.mk	1.1.1.3"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs sysadm.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 sysadm.str

lintit : 

clean :
	rm -f sysadm.str

clobber : clean

