#	copyright	"%c%"

#ident	"@(#)messages:common/cmd/messages/uxmp/msgs.mk	1.1"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs mp.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 mp.str

lintit : 

clean :
	rm -f mp.str

clobber : clean

