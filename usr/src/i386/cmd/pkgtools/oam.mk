#		copyright	"%c%"

#ident	"@(#)oam.mk	15.1"

include $(CMDRULES)

DIRS = libinst pkgmk pkgtrans

all clobber install clean strip lintit:
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) $(MAKEARGS) $@ ;\
		 	cd .. ;\
		fi ;\
	done

