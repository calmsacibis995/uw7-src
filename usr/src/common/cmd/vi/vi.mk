#	copyright	"%c%"

#ident	"@(#)vi:vi.mk	1.33.1.4"
#ident  "$Header$"

include $(CMDRULES)

#	Makefile for vi

OWN = root
GRP = sys

MKDIR = port
MKFILE = makefile.usg     

all : 
	@echo "\n\t>Making commands."
	cd misc; $(MAKE) $(MAKEARGS) all; cd ..
	cd $(MKDIR); $(MAKE) -f $(MKFILE) $(MAKEARGS) all ; cd ..
	@echo "Finished compiling..."

install: all
	if [ ! -d $(ETC)/init.d ] ; then mkdir $(ETC)/init.d ; fi
	if [ ! -d $(ETC)/rc2.d ] ; then mkdir $(ETC)/rc2.d ; fi
	cd misc; $(MAKE) $(MAKEARGS) install
	@echo "\n\t> Installing ex object."
	cd $(MKDIR) ; $(MAKE) -f $(MKFILE) $(MAKEARGS) install
	@echo "\n\t> Creating PRESERVE scripts."
	 $(INS) -f $(ETC)/init.d -m 0444 -u $(OWN) -g $(GRP) PRESERVE
	-rm -f $(ETC)/rc2.d/S02PRESERVE
	-ln -f $(ETC)/init.d/PRESERVE $(ETC)/rc2.d/S02PRESERVE

#
# Cleanup procedures
#
clean lintit:
	cd misc; $(MAKE) $(MAKEARGS) $@ ; cd ..
	cd $(MKDIR); $(MAKE) -f $(MKFILE) $(MAKEARGS) $@ ; cd ..

clobber: clean
	cd misc; $(MAKE) $(MAKEARGS) clobber ; cd ..
	cd $(MKDIR); $(MAKE) -f $(MKFILE) $(MAKEARGS) clobber ; cd ..

#	These targets are useful but optional

partslist productdir product srcaudit:
	cd misc ; $(MAKE) $(MAKEARGS) $@ ; cd ..
	cd $(MKDIR) ; $(MAKE) -f $(MKFILE) $(MAKEARGS) $@ ; cd ..
