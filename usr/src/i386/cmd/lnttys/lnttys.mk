
#ident	"@(#)lnttys.mk	1.2"
#ident "$Header$"

include $(CMDRULES)

OWN = root
GRP = root

all: lnttys.sh
	cp lnttys.sh lnttys

install: all
	$(INS) -f $(USRSBIN) -m 0744 -u $(OWN) -g $(GRP) lnttys

clean:

clobber: clean
	rm -f lnttys
