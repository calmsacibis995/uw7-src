#	copyright	"%c%"

#ident	"@(#)false:common/cmd/false/false.mk	1.4.7.3"
#ident "$Header$"

include $(CMDRULES)

OWN = bin
GRP = bin

all:
	cp false.sh false
	chmod 0755 false

install:	all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) false

clean:

clobber:	clean
	rm -f false
