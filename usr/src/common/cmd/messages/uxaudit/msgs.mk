#	copyright	"%c%"

#ident	"@(#)messages:common/cmd/messages/uxaudit/msgs.mk	1.1.1.3"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs audit.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 audit.str

lintit : 

clean :
	rm -f audit.str

clobber : clean

