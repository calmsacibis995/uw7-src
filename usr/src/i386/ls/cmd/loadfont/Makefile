#	copyright	"%c%"

#ident	"@(#)Makefile	1.2"

include $(CMDRULES)

all: loadfont

clean:
	-rm -f loadfont

clobber: 	clean

install:	all
	$(INS) -f $(ROOT)/$(MACH)/sbin -m 555 -u bin -g bin loadfont
