#ident	"@(#)libdshm:i386/lib/libdshm/libdshm.mk	1.4"

# Makefile for libdsm

include $(LIBRULES)

all:
	cd src; $(MAKE) -f src.mk

install: all
	cd src; $(MAKE) -f src.mk install

clean:	
	cd src; $(MAKE) -f src.mk clean	

clobber: clean
