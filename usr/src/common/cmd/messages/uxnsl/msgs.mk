#	copyright	"%c%"

#ident	"@(#)msgs.mk	1.2"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs nsl.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 nsl.str

lintit : 

clean :
	rm -f nsl.str

clobber : clean

