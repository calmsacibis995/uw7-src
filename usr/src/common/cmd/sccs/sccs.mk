#ident	"@(#)sccs:sccs.mk	6.7.4.5"

include $(CMDRULES)

HELPLIB=$(CCSLIB)/help

ENVPARAMS = CMDRULES="$(CMDRULES)" HELPLIB="$(HELPLIB)"

.MUTEX: libs cmds helplib
all: libs cmds helplib 
	@echo "SCCS is built"

lintit: 
	cd lib; $(MAKE) -f lib.mk $(ENVPARAMS) lintit
	cd cmd; $(MAKE) -f cmd.mk $(ENVPARAMS) lintit
	@echo "SCCS is linted"

libs:
	cd lib; $(MAKE) -f lib.mk $(ENVPARAMS)

cmds:
	cd cmd; $(MAKE) -f cmd.mk $(ENVPARAMS)

helplib:
	cd help.d; $(MAKE) -f help.mk $(ENVPARAMS)

install:
	cd lib; $(MAKE) -f lib.mk $(ENVPARAMS) install
	cd cmd; $(MAKE) -f cmd.mk $(ENVPARAMS) $(ARGS) install
	cd help.d; $(MAKE) -f help.mk $(ENVPARAMS) install

clean:
	cd lib; $(MAKE) -f lib.mk $(ENVPARAMS) clean
	cd cmd; $(MAKE) -f cmd.mk $(ENVPARAMS) clean
	cd help.d; $(MAKE) -f help.mk $(ENVPARAMS) clean

clobber:
	cd lib; $(MAKE) -f lib.mk $(ENVPARAMS) clobber
	cd cmd; $(MAKE) -f cmd.mk $(ENVPARAMS) clobber
	cd help.d; $(MAKE) -f help.mk $(ENVPARAMS) clobber 
