#copyright	"%c%"

#ident	"@(#)top.mk	15.1"

include $(CMDRULES)

DIRS = include libadm libcmd libpkg oampkg

all:
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -P $(MAKEARGS) $@ ;\
		 	cd .. ;\
		fi ;\
	done

