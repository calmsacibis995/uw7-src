#	copyright	"%c%"
#
#ident	"@(#)fmli:msg/msg.mk	1.1"
#

include $(CMDRULES)

MSGS = fmli.str

fmli.str:	uxfmli.src
	-cp uxfmli.src $(MSGS)

all:	$(MSGS)

install: all
	-[ -d $(USRLIB)/locale/C/MSGFILES ] || \
			mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 fmli.str

clean: 
	-rm -f $(MSGS)

clobber:	clean
