#	copyright	"%c%"

#ident	"@(#)msgs.mk	1.2"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs nsu.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 nsu.str

lintit : 

clean :
	rm -f nsu.str

clobber : clean

