#ident	"@(#)cscope:cscope.mk	1.21"

include $(CMDRULES)

ENVPARMS= \
	CMDRULES="$(CMDRULES)" 

all:
	cd $(CPU) ; $(MAKE) all

install:
	cd $(CPU) ; $(MAKE) install ROOT=$(ROOT)

lintit:
	cd $(CPU) ; $(MAKE) lintit $(ENVPARMS)

clean:
	cd $(CPU) ; $(MAKE) clean

clobber:
	cd $(CPU) ; $(MAKE) clobber
