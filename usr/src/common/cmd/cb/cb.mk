#ident	"@(#)cb:cb.mk	1.10"

include $(CMDRULES)

LDLIBS=
CMDBASE=../..

ENVPARMS= \
	CMDRULES="$(CMDRULES)" LDLIBS="$(LDLIBS)" CMDBASE="$(CMDBASE)"

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
