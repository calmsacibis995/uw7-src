#ident	"@(#)ctrace:ctrace.mk	1.17.1.8"

include $(CMDRULES)

ENVPARMS= \
	CMDRULES="$(CMDRULES)" 

all:
	cd $(CPU) ; $(MAKE) all $(ENVPARMS)

install:
	cd $(CPU) ; $(MAKE) install $(ENVPARMS)


lintit:
	cd $(CPU) ; $(MAKE) lintit $(ENVPARMS)

clean:
	cd $(CPU) ; $(MAKE) clean

clobber:
	cd $(CPU) ; $(MAKE) clobber
