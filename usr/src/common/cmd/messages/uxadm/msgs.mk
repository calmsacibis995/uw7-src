#	copyright	"%c%"

#ident	"@(#)messages:common/cmd/messages/uxadm/msgs.mk	1.1"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs adm.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 adm.str

lintit : 

clean :
	rm -f adm.str

clobber : clean

