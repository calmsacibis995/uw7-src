#ident	"@(#)mt:mt.mk	1.1"

include $(CMDRULES)

OWN = bin
GRP = bin

MAINS = mt

SOURCES =  mt.sh

all:		$(MAINS)

mt:		mt.sh
	cp mt.sh mt

clobber: 
	rm -f $(MAINS)

install: all
	 $(INS) -f  $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) $(MAINS)

clean:

lintit:

