#ident	"@(#)cxref:cxref.mk	1.24"

include $(CMDRULES)

LDLIBS=
ENVPARMS= \
	CMDRULES="$(CMDRULES)" LDLIBS="$(LDLIBS)"

all:
	cd $(CPU) ; $(MAKE) all $(ENVPARMS)

install:
	cd $(CPU) ; $(MAKE) install

lintit:
	cd $(CPU) ; $(MAKE) lintit $(ENVPARMS)

clean:
	cd $(CPU) ; $(MAKE) clean

clobber:
	cd $(CPU ) ; $(MAKE) clobber
