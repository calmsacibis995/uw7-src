#	copyright	"%c%"

#ident	"@(#)msgs.mk	1.2"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs bnu.abi.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 bnu.abi.str

lintit : 

clean :
	rm -f bnu.abi.str

clobber : clean

