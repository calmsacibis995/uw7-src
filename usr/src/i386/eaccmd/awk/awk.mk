#	copyright	"%c%"

#ident	"@(#)eac:i386/eaccmd/awk/awk.mk	1.4.1.1"
#ident  "$Header$"

include $(CMDRULES)

USREAC=$(USR)/eac

all clean clobber lintit:  

install: all
	- [ -d $(USREAC)/bin ] || mkdir -p $(USREAC)/bin
	rm -f $(USREAC)/bin/awk
	$(SYMLINK) /usr/bin/nawk $(USREAC)/bin/awk
