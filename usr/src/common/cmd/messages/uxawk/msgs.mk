#	copyright	"%c%"

#ident	"@(#)messages:common/cmd/messages/uxawk/msgs.mk	1.1.3.3"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs awk.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 awk.str

lintit : 

clean :
	rm -f awk.str

clobber : clean

