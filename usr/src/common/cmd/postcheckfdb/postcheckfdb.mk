#	copyright	"1"

#ident	"@(#)postcheckfdb:postcheckfdb.mk	1.1"
#	Makefile for postcheckfdb

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/X/adm

MAINS = postcheckfdb

SOURCES =  postcfdb.sh

all:	$(MAINS)

postcheckfdb:	 $(SOURCES)
	cp postcfdb.sh postcheckfdb

clean:
	rm -f $(MAINS)

clobber: clean

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 $(MAINS)

