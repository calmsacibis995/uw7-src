#	copyright	"%c%"

#ident	"@(#)messages:common/cmd/messages/uxes/msgs.mk	1.6"

include $(CMDRULES)


all	: msgs

install	: all 
	cp msgs es.str
	if [ ! -d $(USRLIB)/locale/C/MSGFILES ]; \
		then mkdir -p $(USRLIB)/locale/C/MSGFILES; fi; \
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 0644 es.str

lintit	: 

clean	:
	rm -f es.str

clobber	: clean

