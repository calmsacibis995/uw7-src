#ident	"@(#)cflow:cflow.mk	1.12.2.12"

include $(CMDRULES)

CMDBASE=..
ENVPARMS= \
	CMDRULES='$(CMDRULES)'

all:
	cd $(CPU) ; $(MAKE) all $(ENVPARMS)

install:
	cd $(CPU) ; $(MAKE) install

lintit:
	cd $(CPU) ; $(MAKE) lintit $(ENVPARMS)

clean:
	cd $(CPU) ; $(MAKE) clean

clobber:
	cd $(CPU) ; $(MAKE) clobber
