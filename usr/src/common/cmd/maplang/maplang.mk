#	copyright	"1"

#ident	"@(#)maplang.mk	1.2"
#	Makefile for maplang

include $(CMDRULES)

INSDIR = $(USRSADM)/install/bin

MAINS = maplang

SOURCES =  maplang.sh

all:	$(MAINS)

maplang:	 $(SOURCES)
	cp maplang.sh maplang

clean:
	rm -f $(MAINS)

clobber: clean

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 $(MAINS)

