#	copyright	"%c%"

#ident	"@(#)messages:common/cmd/messages/uxels/msgs.mk	1.1"

include $(CMDRULES)


all	: msgs

install	: all 
	cp msgs els.str
	if [ ! -d $(USRLIB)/locale/C/MSGFILES ]; \
		then mkdir -p $(USRLIB)/locale/C/MSGFILES; fi; \
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 0644 els.str

lintit	: 

clean	:
	rm -f els.str

clobber	: clean

