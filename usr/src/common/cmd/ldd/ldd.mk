#ident	"@(#)ldd:ldd.mk	1.9"

# makefile for ldd (List Dynamic Dependencies)

include $(CMDRULES)

CMDBASE=..
ENVPARMS=CMDRULES="$(CMDRULES)" CMDBASE="$(CMDBASE)"

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
