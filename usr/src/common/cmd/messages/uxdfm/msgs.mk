#	copyright	"%c%"

#ident	"@(#)messages:common/cmd/messages/uxdfm/msgs.mk	1.1.3.3"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs dfm.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 dfm.str

lintit : 

clean :
	rm -f dfm.str

clobber : clean

