#	copyright	"%c%"

#ident	"@(#)kill:kill.mk	1.12.7.2"
#ident  "$Header$"

include $(CMDRULES)


OWN = bin
GRP = bin

all: kill

kill:
	echo \#\!/sbin/sh > kill
	echo /sbin/sh -c \"kill $$\*\" >> kill

clean:

clobber: clean
	rm -f kill

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) kill
